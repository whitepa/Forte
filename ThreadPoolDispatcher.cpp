#include "ThreadPoolDispatcher.h"
#include "LogManager.h"
#include "Foreach.h"
#include <boost/scoped_ptr.hpp>
using namespace boost;

////////////////////////////// Thread pool dispatcher

Forte::ThreadPoolDispatcherManager::ThreadPoolDispatcherManager(ThreadPoolDispatcher &disp) :
    DispatcherThread(disp)
{
    initialized();
}

void * Forte::ThreadPoolDispatcherManager::run(void)
{
    ThreadPoolDispatcher &disp(dynamic_cast<ThreadPoolDispatcher&>(mDispatcher));
    mThreadName.Format("%s-disp-%u", disp.mDispatcherName.c_str(), (unsigned)mThread);
    
    // start initial worker threads
    for (unsigned int i = 0; i < disp.mMinThreads; ++i)
    {
        disp.mThreadSem.wait();
        try
        {
            new ThreadPoolDispatcherWorker(disp);
        }
        catch (...)
        {
            // XXX log
            disp.mThreadSem.post();
            break;
        }
    }
    // loop until shutdown
    int lastNew = 0;
    while (!disp.mShutdown)
    {
        // see if new threads are needed
        sleep(1);
        int currentThreads = disp.mMaxThreads - disp.mThreadSem.getvalue();
        int spareThreads = disp.mSpareThreadSem.getvalue();
        int newThreadsNeeded = disp.mEventQueue.depth() + disp.mMinSpareThreads - spareThreads;
        int numNew = 0;
        if (newThreadsNeeded < (int)disp.mMinThreads - currentThreads)
            newThreadsNeeded = (int)disp.mMinThreads - currentThreads;
//        hlog(HLOG_DEBUG4, "ThreadPool Manager Loop: %d threads; %d spare; %d needed",
//             currentThreads, spareThreads, newThreadsNeeded);
        if (!disp.mShutdown && newThreadsNeeded > 0)
        {
            numNew = (lastNew == 0) ? 1 : lastNew * 2;
            // we must now create numNew threads
            for (int i = 0; i < numNew; ++i)
            {
                if (disp.mThreadSem.trywait() == -1 && errno == EAGAIN)
                {
                    numNew = 0;
                    break;
                }
                else
                {
                    // create a new worker thread
                    try {
                        new ThreadPoolDispatcherWorker(disp);
                    } catch (...) {
                        // XXX log err
                        disp.mThreadSem.post();
                        numNew = 0;
                        break;
                    }
                }
            }
        }
        else if (spareThreads > (int)disp.mMaxSpareThreads &&
                 spareThreads > (int)disp.mMinSpareThreads)
        {
            // too many spare threads, shut one down
//            hlog(HLOG_DEBUG, "shutting down a thread");
            AutoUnlockMutex lock(disp.mThreadsLock);
            std::vector<shared_ptr<DispatcherThread> >::iterator i = disp.mThreads.begin();
            if (*i)
                (*i)->shutdown();
        }
        lastNew = (numNew > 0) ? numNew : 0;
    }

    // shut down
    hlog(HLOG_DEBUG, "'%s' dispatcher shutting down, handling final queued requests...",
         disp.mDispatcherName.c_str());

    // wait until all requests are processed
    disp.mEventQueue.waitUntilEmpty();
    
    hlog(HLOG_DEBUG, "'%s' dispatcher shutting down, waiting for threads to exit...",
         disp.mDispatcherName.c_str());

    // tell all workers to shutdown
    {
        AutoUnlockMutex lock(disp.mThreadsLock);
        foreach (shared_ptr<DispatcherThread> &thr, disp.mThreads)
        {
            thr->shutdown();
        }
        // delete all the threads
        disp.mThreads.clear();
        // TODO: timed deletion of all threads, give up after a while
    }
    hlog(HLOG_DEBUG, "'%s' dispatcher shutdown complete", disp.mDispatcherName.c_str());
    return NULL;
}

Forte::ThreadPoolDispatcherWorker::ThreadPoolDispatcherWorker(ThreadPoolDispatcher &disp) :
    DispatcherThread(disp),
    mLastPeriodicCall(time(0))
{
    initialized();
}
Forte::ThreadPoolDispatcherWorker::~ThreadPoolDispatcherWorker()
{
    ThreadPoolDispatcher &disp(dynamic_cast<ThreadPoolDispatcher&>(mDispatcher));
    disp.mRequestHandler.cleanup();
    disp.unregisterThread(this);
    disp.mThreadSem.post();
}
void * Forte::ThreadPoolDispatcherWorker::run(void)
{
    ThreadPoolDispatcher &disp(dynamic_cast<ThreadPoolDispatcher&>(mDispatcher));
    mThreadName.Format("%s-pool-%u", mDispatcher.mDispatcherName.c_str(), (unsigned)mThread);
    
    // call the request handler's initialization hook
    disp.mRequestHandler.init();

    // pull events from the request queue
    while (!mThreadShutdown)
    {
        AutoUnlockMutex lock(disp.mNotifyLock);
        while (disp.mPaused == false &&
               !mThreadShutdown)
        {
            shared_ptr<Event> event(disp.mEventQueue.get());
            if (!event) break;
            // set event pointer here
            mEventPtr = event;
            // set start time
            gettimeofday(&(mEventPtr->mStartTime), NULL);
            {
                AutoLockMutex unlock(disp.mNotifyLock);
                // process the request
                try
                {
                    disp.mRequestHandler.handler(event.get());
                }
                catch (Exception &e)
                {
                    hlog(HLOG_ERR, "exception thrown in event handler: %s",
                         e.getDescription().c_str());
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
            mEventPtr.reset();
            // event will be deleted HERE
        }
        struct timeval now;
        struct timespec timeout;
        gettimeofday(&now, 0);
        timeout.tv_sec = now.tv_sec + 1; // wake up every second
        timeout.tv_nsec = now.tv_usec * 1000;
        disp.mSpareThreadSem.post();
        disp.mNotify.timedwait(timeout);
        disp.mSpareThreadSem.trywait();
        if (mThreadShutdown) break;
        // see if we need to run periodic
        gettimeofday(&now, 0);
        if (disp.mRequestHandler.mTimeout != 0 &&
            (unsigned int)(now.tv_sec - mLastPeriodicCall) > disp.mRequestHandler.mTimeout)
        {
            disp.mRequestHandler.periodic();
            mLastPeriodicCall = now.tv_sec;
        }
    }
    return NULL;
}

Forte::ThreadPoolDispatcher::ThreadPoolDispatcher(RequestHandler &requestHandler,
                                                  const int minThreads, const int maxThreads, 
                                                  const int minSpareThreads, const int maxSpareThreads,
                                                  const int deepQueue, const int maxDepth, const char *name):
    Dispatcher(requestHandler, maxDepth, name),
    mMinThreads(minThreads),
    mMaxThreads(maxThreads),
    mMinSpareThreads(minSpareThreads),
    mMaxSpareThreads(maxSpareThreads),
    mThreadSem(maxThreads),
    mSpareThreadSem(0),
    mManagerThread(*this)
{
}
Forte::ThreadPoolDispatcher::~ThreadPoolDispatcher()
{
    // stop accepting new events
    mEventQueue.shutdown();
    // set the shutdown flag
    mShutdown = true;
    // wait for the manager thread to exit!
    // (this allows all worker threads to safely exit and unregister themselves
    //  before this destructor exits; otherwise bad shit happens when this object
    //  is dealloced and worker threads are still around trying to access data)
    mManagerThread.waitForShutdown();
}
void Forte::ThreadPoolDispatcher::pause(void) { mPaused = 1; }
void Forte::ThreadPoolDispatcher::resume(void) { mPaused = 0; mNotify.broadcast(); }
void Forte::ThreadPoolDispatcher::enqueue(shared_ptr<Event> e)
{ 
    if (mShutdown)
        throw ForteThreadPoolDispatcherException("dispatcher is shutting down; no new events are being accepted");
    mEventQueue.add(e); 
}
bool Forte::ThreadPoolDispatcher::accepting(void) { return mEventQueue.accepting(); }

int Forte::ThreadPoolDispatcher::getRunningEvents(int maxEvents,
                                                  std::list<shared_ptr<Event> > &runningEvents)
{
    // loop through the dispatcher threads
    AutoUnlockMutex lock(mNotifyLock);
    AutoUnlockMutex thrLock(mThreadsLock);
    
    int count = 0;
    foreach (shared_ptr<DispatcherThread> dt, mThreads)
    {
        if (dt->mEventPtr)
        {
            runningEvents.push_back(dt->mEventPtr);
            ++count;
        }
    }
    return count;
}
