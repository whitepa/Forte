#include "OnDemandDispatcher.h"
#include "LogManager.h"
#include "Foreach.h"
#include "FTrace.h"

using namespace boost;
using namespace Forte;

/////////////////////////// On demand dispatcher

Forte::OnDemandDispatcherManager::OnDemandDispatcherManager(
    OnDemandDispatcher &disp)
    : DispatcherThread(disp)
{
    FTRACE;
    initialized();
}

void * Forte::OnDemandDispatcherManager::run(void)
{
    FTRACE;

    OnDemandDispatcher &disp(dynamic_cast<OnDemandDispatcher&>(mDispatcher));
    mThreadName = "dsp-od";

    AutoUnlockMutex lock(disp.mNotifyLock);

    while (1) // always check for queued events before shutting down
    {
        // reap any completed threads TODO: may need a better method
        // of doing this.  Once a second may not be enough in a busy
        // environment.
        {
            AutoUnlockMutex thrLock(disp.mThreadsLock);
            std::vector<boost::shared_ptr<DispatcherWorkerThread> >::iterator i;
            i = disp.mThreads.begin();
            while(i != disp.mThreads.end())
            {
                boost::shared_ptr<DispatcherWorkerThread> element = *i;
                // OnDemandDispatcher workers are done with the work
                // handler if HasEvent() returns false.
                if (!element->HasEvent())
                {
                    // this WaitForShutdown should be near
                    // instantaneous. if this needs to be moved out of
                    // the main loop that would an optimization.
                    // without this here, thr can become invalid in
                    // Thread::startThread when thr->Shutdown is called
                    element->WaitForShutdown();
                    i = disp.mThreads.erase(i);
                }
                else
                {
                    i++;
                }
            }
        }

        boost::shared_ptr<Event> event;
        while (!disp.mPaused && (event = disp.mEventQueue.Get()))
        {
            AutoLockMutex unlock(disp.mNotifyLock);
            disp.mThreadSem.Wait();

            AutoUnlockMutex thrLock(disp.mThreadsLock);
            disp.mThreads.push_back(
                boost::shared_ptr<DispatcherWorkerThread>(
                    new OnDemandDispatcherWorker(disp, event)));

            hlog(HLOG_DEBUG2, "Number of threads in queue : %d",
                 static_cast<int>(disp.mThreads.size()));
        }

        if (disp.mShutdown)
        {
            hlog(HLOG_DEBUG, "Going to shutdown dispatcher thread");
            break;
        }

        disp.mNotify.TimedWait(1);
    }

    // signal any workers to shutdown
    {
        AutoUnlockMutex thrlock(disp.mThreadsLock);
        foreach (boost::shared_ptr<DispatcherWorkerThread> &thr, disp.mThreads)
        {
            if (thr)
                thr->Shutdown();
        }

        foreach (boost::shared_ptr<DispatcherWorkerThread> &thr, disp.mThreads)
        {
            if (thr)
                thr->WaitForShutdown();
        }


        // delete all threads
        disp.mThreads.clear();
    }

    return NULL;
}

Forte::OnDemandDispatcherWorker::OnDemandDispatcherWorker(
    OnDemandDispatcher &disp,
    const boost::shared_ptr<Event>& event)
    : DispatcherWorkerThread(disp, event)
{
    FTRACE;
    initialized();
}

Forte::OnDemandDispatcherWorker::~OnDemandDispatcherWorker()
{
    FTRACE;
    OnDemandDispatcher &disp(dynamic_cast<OnDemandDispatcher&>(mDispatcher));
    disp.mRequestHandler->Cleanup();
    disp.mThreadSem.Post();

    deleting();
}

void * Forte::OnDemandDispatcherWorker::run()
{
    FTRACE;
    OnDemandDispatcher &disp(dynamic_cast<OnDemandDispatcher&>(mDispatcher));
    mThreadName.Format("%s-od", disp.mDispatcherName.c_str());
    disp.mRequestHandler->Init();
    disp.mRequestHandler->Handler(getRawEventPointer());
    clearEvent();
    // thread is complete at this point, resetting our event pointer
    // will cause the manager to reap us

    return NULL;
}

Forte::OnDemandDispatcher::OnDemandDispatcher(
    boost::shared_ptr<RequestHandler> requestHandler,
    const int maxThreads,
    const int deepQueue,
    const int maxDepth,
    const char *name)
    : Dispatcher(requestHandler, maxDepth, name),
      mMaxThreads(maxThreads),
      mThreadSem(maxThreads),
      mManagerThread(*this)
{
    FTRACE;
    mDispatcherName = name;
}

Forte::OnDemandDispatcher::~OnDemandDispatcher()
{
    FTRACE;

    if (!IsShuttingDown())
    {
        try
        {
            Shutdown();
        }
        catch(...)
        {
            hlog(HLOG_ERR, "Exception thrown shutting down");
        }
    }
}

void Forte::OnDemandDispatcher::Shutdown()
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

void Forte::OnDemandDispatcher::Pause(void)
{
    FTRACE;
    AutoUnlockMutex lock(mNotifyLock);
    mPaused = 1;
}

void Forte::OnDemandDispatcher::Resume(void)
{
    FTRACE;
    AutoUnlockMutex lock(mNotifyLock);
    mPaused = 0;
    mNotify.Signal();
}

void Forte::OnDemandDispatcher::Enqueue(boost::shared_ptr<Event> e)
{
    mEventQueue.Add(e);
}

bool Forte::OnDemandDispatcher::Accepting(void)
{
    return mEventQueue.Accepting();
}

int Forte::OnDemandDispatcher::GetRunningEvents(
    int maxEvents,
    std::list<boost::shared_ptr<Event> > &runningEvents)
{
    // lock the notify lock to prevent threads from freeing events while
    // we are copying them
    AutoUnlockMutex lock(mNotifyLock);
    AutoUnlockMutex thrLock(mThreadsLock);
    int count = 0;
    // loop through the dispatcher threads
    foreach (boost::shared_ptr<DispatcherWorkerThread> &thr, mThreads)
    {
        // copy each event
        if (thr && thr->mEventPtr)
        {
            runningEvents.push_back(thr->mEventPtr);
            ++count;
        }
    }
    return count;
}

bool Forte::OnDemandDispatcher::StopRunningEvent(boost::shared_ptr<Event> &runningEvent)
{
    AutoUnlockMutex lock(mNotifyLock);
    AutoUnlockMutex thrLock(mThreadsLock);

    foreach (boost::shared_ptr<DispatcherWorkerThread> &thr, mThreads)
    {
        // copy each event
        if (thr && (thr->mEventPtr == runningEvent))
        {
            // found it, shutdown the thread
            thr->Shutdown();
            return true;
        }
    }

    // if we get here, couldn't find the event in the running threads
    return false;
}
