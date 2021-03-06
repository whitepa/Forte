#include "ThreadPoolDispatcher.h"
#include "LogManager.h"
#include "Foreach.h"
#include "Clock.h"
#include <boost/scoped_ptr.hpp>
#include "FTrace.h"

using namespace boost;
using namespace Forte;

////////////////////////////// Thread pool dispatcher

Forte::ThreadPoolDispatcherManager::ThreadPoolDispatcherManager(
    ThreadPoolDispatcher &disp)
    : DispatcherThread(disp)
{
    FTRACE;
    initialized();
}

void* Forte::ThreadPoolDispatcherManager::run(void)
{
    FTRACE;

    ThreadPoolDispatcher &disp(dynamic_cast<ThreadPoolDispatcher&>(mDispatcher));
    mThreadName.Format("%s-disp-%u", disp.mDispatcherName.c_str(),
                       GetThreadID());

    // start initial worker threads
    for (unsigned int i = 0; i < disp.mMinThreads; ++i)
    {
        disp.mThreadSem.Wait();
        try
        {
            hlog(HLOG_DEBUG2, "%s: Creating %i (of %i) thread pool worker ...",
                 disp.mDispatcherName.c_str(), i, disp.mMinThreads);
            AutoUnlockMutex lock(disp.mThreadsLock);
            disp.mThreads.push_back(boost::shared_ptr<ThreadPoolDispatcherWorker>(
                                        new ThreadPoolDispatcherWorker(disp)));
        }
        catch (...)
        {
            hlog(HLOG_WARN, "Error while creating new thread pool worker");
            disp.mThreadSem.Post();
            break;
        }
    }
    // loop until shutdown
    int lastNew = 0;
    while (!disp.IsShuttingDown())
    {
        interruptibleSleep(Timespec::FromMillisec(1000), false);

        // see if new threads are needed
        int currentThreads = disp.mMaxThreads - disp.mThreadSem.GetValue();
        int spareThreads = disp.mSpareThreadSem.GetValue();
        int newThreadsNeeded =
            disp.mEventQueue.Depth() + disp.mMinSpareThreads - spareThreads;
        int numNew = 0;
        if (newThreadsNeeded < static_cast<int>(disp.mMinThreads) - currentThreads)
            newThreadsNeeded = static_cast<int>(disp.mMinThreads) - currentThreads;
        //hlog(HLOG_DEBUG4,
        //"ThreadPool Manager Loop: %d threads; %d spare; %d needed",
        //currentThreads, spareThreads, newThreadsNeeded);
        if (!disp.IsShuttingDown() && newThreadsNeeded > 0)
        {
            numNew = (lastNew == 0) ? 1 : lastNew * 2;

            // we must now create numNew threads
            for (int i = 0; i < numNew; ++i)
            {
                if (disp.mThreadSem.TryWait() == -1 && errno == EAGAIN)
                {
                    numNew = 0;
                    break;
                }
                else
                {
                    // create a new worker thread
                    try
                    {
                        hlog(HLOG_DEBUG2, "%s: Creating thread pool worker %i",
                             disp.mDispatcherName.c_str(), i);

                        AutoUnlockMutex lock(disp.mThreadsLock);
                        disp.mThreads.push_back(
                            boost::shared_ptr<ThreadPoolDispatcherWorker>(
                                new ThreadPoolDispatcherWorker(disp)));
                    }
                    catch (...)
                    {
                        // XXX log err
                        hlog(HLOG_WARN,
                             "Error occurred while creating new worker");
                        disp.mThreadSem.Post();
                        numNew = 0;
                        break;
                    }
                }
            }
        }
        else if (spareThreads > static_cast<int>(disp.mMaxSpareThreads) &&
                 spareThreads > static_cast<int>(disp.mMinSpareThreads))
        {
            // too many spare threads, shut one down
            hlog(HLOG_DEBUG2, "have %d spare threads, need between %u and %u",
                 spareThreads, disp.mMinSpareThreads, disp.mMaxSpareThreads);

            AutoUnlockMutex lock(disp.mThreadsLock);
            std::vector<boost::shared_ptr<DispatcherWorkerThread> >::iterator i =
                disp.mThreads.begin();
            //TODO? should thread pool dispatcher try to kill idle
            //threads first?
            if (*i)
            {
                (*i)->Shutdown();
            }
            disp.mThreads.erase(i);
        }
        lastNew = (numNew > 0) ? numNew : 0;
    }

    // shut down
    hlog(HLOG_DEBUG2,
         "'%s' dispatcher shutting down, handling final queued requests...",
         disp.mDispatcherName.c_str());

    // wait until all requests are processed
    disp.mEventQueue.WaitUntilEmpty();

    hlog(HLOG_DEBUG2,
         "'%s' dispatcher shutting down, waiting for threads to exit...",
         disp.mDispatcherName.c_str());

    // tell all workers to shutdown
    {
        AutoUnlockMutex lock(disp.mThreadsLock);
        foreach (boost::shared_ptr<DispatcherWorkerThread> &thr, disp.mThreads)
        {
            if (thr)
            {
                hlog(HLOG_DEBUG3, "notifying thread %s of shutdown",
                     thr->mThreadName.c_str());
                thr->Shutdown();
            }
            else
                hlog(HLOG_ERR,
                     "invalid thread pointer in dispatcher thread vector");
        }
    }
    // since the threads may now be waiting on the main dispatcher
    // notification condition, broadcast it (and avoid holding both locks)
    {
        AutoUnlockMutex lock(disp.mNotifyLock);
        disp.mNotify.Broadcast();
    }

    {
        // now delete all the threads
        AutoUnlockMutex lock(disp.mThreadsLock);

        // our dispatcher is waiting for us to shutdown. our workers
        // have a reference to the dispatcher. we have to ensure our
        // workers exit before we do so their Dispatcher& does not
        // become invalid
        foreach (boost::shared_ptr<DispatcherWorkerThread> &thr, disp.mThreads)
        {
            if (thr)
            {
                hlog(HLOG_DEBUG3, "waiting for thread %s to shutdown",
                     thr->mThreadName.c_str());
                thr->WaitForShutdown();
            }
            else
                hlog(HLOG_ERR,
                     "invalid thread pointer in dispatcher thread vector");
        }

        disp.mThreads.clear();
        // TODO: timed deletion of all threads, give up after a while
    }
    hlog(HLOG_DEBUG2, "'%s' dispatcher shutdown complete", disp.mDispatcherName.c_str());
    return NULL;
}

Forte::ThreadPoolDispatcherWorker::ThreadPoolDispatcherWorker(
    ThreadPoolDispatcher &disp)
    : DispatcherWorkerThread(
        disp,
        boost::shared_ptr<Event>() // work will pull events from queue
        ),
      mMonotonicClock(),
      mLastPeriodicCall(mMonotonicClock.GetTime().AsSeconds())
{
    FTRACE;
    initialized();
}

Forte::ThreadPoolDispatcherWorker::~ThreadPoolDispatcherWorker()
{
    FTRACE;

    // This method will be called in the thread which is destroying
    // the worker, NOT by the worker thread itself.
//    hlog(HLOG_DEBUG3, "worker cleanup");
    ThreadPoolDispatcher &disp(dynamic_cast<ThreadPoolDispatcher&>(mDispatcher));
    //disp.mRequestHandler->Cleanup(); // TODO: must be called by
    //worker thread itself

    disp.mThreadSem.Post();
    // TODO: move to manager?  or remove threadSem altogether, since
    //the size of the thread vector should now be an accurate
    //representation of the number of threads.

    deleting();
}

void * Forte::ThreadPoolDispatcherWorker::run(void)
{
    FTRACE;

    ThreadPoolDispatcher &disp(dynamic_cast<ThreadPoolDispatcher&>(mDispatcher));
    mThreadName.Format("%s-pool", mDispatcher.mDispatcherName.c_str());
    hlog(HLOG_DEBUG3, "initializing...");
    // call the request handler's initialization hook
    disp.mRequestHandler->Init();

    // pull events from the request queue
    while (!Thread::IsShuttingDown())
    {
        AutoUnlockMutex lock(disp.mNotifyLock);
        while (disp.mPaused == false &&
               !Thread::IsShuttingDown())
        {
            boost::shared_ptr<Event> event(disp.mEventQueue.Get());
            if (!event) break;
            setEvent(event);
            // set start time
            gettimeofday(&(event->mStartTime), NULL);
            {
                AutoLockMutex unlock(disp.mNotifyLock);
                // process the request
                try
                {
                    disp.mRequestHandler->Handler(getRawEventPointer());
                }
                catch (EThreadShutdown &e)
                {
                    //normal
                }
                catch (EThreadPoolDispatcherShuttingDown &e)
                {
                    // normal
                }
                catch (std::exception &e)
                {
                    hlog(HLOG_ERR, "exception thrown in event handler: %s",
                         e.what());
                }
                catch (...)
                {
                    hlog(HLOG_ERR, "unknown exception thrown in event handler");
                }
            }
            // mNotifyLock is LOCKED here
            // unset event pointer
            clearEvent();
            // event will be deleted HERE
        }
        disp.mSpareThreadSem.Post();
//        hlog(HLOG_DEBUG4, "waiting for events...");
        disp.mNotify.TimedWait(1);  // wake up every second
        disp.mSpareThreadSem.TryWait();
        if (Thread::IsShuttingDown()) break;
        // see if we need to run periodic
        struct timespec now;
        mMonotonicClock.GetTime(now);
        if (disp.mRequestHandler->mTimeout != 0 &&
            static_cast<unsigned int>
            (now.tv_sec - mLastPeriodicCall) > disp.mRequestHandler->mTimeout)
        {
            disp.mRequestHandler->Periodic();
            mLastPeriodicCall = now.tv_sec;
        }
    }
    hlog(HLOG_DEBUG2, "Thread shutdown");
    return NULL;
}

Forte::ThreadPoolDispatcher::ThreadPoolDispatcher(
    boost::shared_ptr<RequestHandler> requestHandler,
    const int minThreads,
    const int maxThreads,
    const int minSpareThreads,
    const int maxSpareThreads,
    const int deepQueue,
    const int maxDepth,
    const char *name)
    : Dispatcher(requestHandler, maxDepth, name),
      mMinThreads(minThreads),
      mMaxThreads(maxThreads),
      mMinSpareThreads(minSpareThreads),
      mMaxSpareThreads(maxSpareThreads),
      mThreadSem(maxThreads),
      mSpareThreadSem(0),
      mManagerThread(*this)
{
    FTRACE;
}

Forte::ThreadPoolDispatcher::~ThreadPoolDispatcher()
{
    FTRACE;
    if (!IsShuttingDown())
        Shutdown();
}

void Forte::ThreadPoolDispatcher::Shutdown(void)
{
    FTRACE;

    Dispatcher::Shutdown();

    // wait for the manager thread to exit!
    // (this allows all worker threads to safely exit and unregister themselves
    //  before this destructor exits; otherwise bad shit happens when this object
    //  is dealloced and worker threads are still around trying to access data)
    mManagerThread.Shutdown();
    mManagerThread.WaitForShutdown();
}

void Forte::ThreadPoolDispatcher::Pause(void)
{
    AutoUnlockMutex lock(mNotifyLock);
    mPaused = 1;
}

void Forte::ThreadPoolDispatcher::Resume(void)
{
    AutoUnlockMutex lock(mNotifyLock);
    mPaused = 0;
    mNotify.Broadcast();
}

void Forte::ThreadPoolDispatcher::Enqueue(boost::shared_ptr<Event> e)
{
    //FTRACE;

    if (IsShuttingDown())
        throw EThreadPoolDispatcherShuttingDown(
            "dispatcher is shutting down; no new events are being accepted");
    mEventQueue.Add(e);
}

bool Forte::ThreadPoolDispatcher::Accepting(void)
{
    return mEventQueue.Accepting();
}

int Forte::ThreadPoolDispatcher::GetRunningEvents(
    int maxEvents,
    std::list<boost::shared_ptr<Event> > &runningEvents)
{
    // loop through the dispatcher threads
    AutoUnlockMutex lock(mNotifyLock);
    AutoUnlockMutex thrLock(mThreadsLock);

    int count = 0;
    foreach (boost::shared_ptr<DispatcherWorkerThread> dt, mThreads)
    {
        if (dt->HasEvent())
        {
            runningEvents.push_back(dt->getEvent());
            ++count;
        }
    }
    return count;
}
