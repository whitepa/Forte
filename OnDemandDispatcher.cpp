#include "OnDemandDispatcher.h"
#include "LogManager.h"
#include "Foreach.h"
#include "FTrace.h"

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
    mThreadName.Format("dsp-od-%u", GetThreadID());

    AutoUnlockMutex lock(disp.mNotifyLock);

    while (1) // always check for queued events before shutting down
    {
        // reap any completed threads TODO: may need a better method
        // of doing this.  Once a second may not be enough in a busy
        // environment.
        {
            AutoUnlockMutex thrLock(disp.mThreadsLock);
            std::vector<shared_ptr<DispatcherThread> >::iterator i;
            i = disp.mThreads.begin();
            while(i != disp.mThreads.end())
            {
                shared_ptr<DispatcherThread> element = *i;
                // OnDemandDispatcher workers are guaranteed to be
                // finished if HasEvent() returns false.
                if (!element->HasEvent())
                {
                    i = disp.mThreads.erase(i);
                } else
                {
                    i++;
                }
            }
        }

        shared_ptr<Event> event;
        while (!disp.mPaused && (event = disp.mEventQueue.Get()))
        {
            AutoLockMutex unlock(disp.mNotifyLock);
            disp.mThreadSem.Wait();

            AutoUnlockMutex thrLock(disp.mThreadsLock);
            disp.mThreads.push_back(
                shared_ptr<DispatcherThread>(
                    new OnDemandDispatcherWorker(disp, event)));

            hlog(HLOG_DEBUG2, "Number of threads in queue : %d",
                 (int) disp.mThreads.size());
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
        foreach (shared_ptr<DispatcherThread> &thr, disp.mThreads)
        {
            if (thr)
                thr->Shutdown();
        }

        // delete all threads
        disp.mThreads.clear();
    }

    return NULL;
}

Forte::OnDemandDispatcherWorker::OnDemandDispatcherWorker(
    OnDemandDispatcher &disp,
    shared_ptr<Event> event)
    : DispatcherThread(disp)
{
    FTRACE;
    mEventPtr = event;
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
    mThreadName.Format("%s-od-%u", disp.mDispatcherName.c_str(), GetThreadID());
    disp.mRequestHandler->Init();
    disp.mRequestHandler->Handler(mEventPtr.get());
    // thread is complete at this point, resetting our event pointer
    // will cause the manager to reap us
    mEventPtr.reset();

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
    // stop accepting new events
    mEventQueue.Shutdown();
    // set the shutdown flag
    mShutdown = true;
    // wait for the manager thread to exit!
    // (this allows all worker threads to safely exit and unregister themselves
    //  before this destructor exits; otherwise bad shit happens when this object
    //  is dealloced and worker threads are still around trying to access data)
    mManagerThread.WaitForShutdown();
}

void Forte::OnDemandDispatcher::Pause(void)
{
    FTRACE;
    mPaused = 1;
}

void Forte::OnDemandDispatcher::Resume(void)
{
    FTRACE;
    mPaused = 0;
    mNotify.Signal();
}

void Forte::OnDemandDispatcher::Enqueue(shared_ptr<Event> e)
{
    mEventQueue.Add(e);
}

bool Forte::OnDemandDispatcher::Accepting(void)
{
    return mEventQueue.Accepting();
}

int Forte::OnDemandDispatcher::GetRunningEvents(
    int maxEvents,
    std::list<shared_ptr<Event> > &runningEvents)
{
    // lock the notify lock to prevent threads from freeing events while
    // we are copying them
    AutoUnlockMutex lock(mNotifyLock);
    AutoUnlockMutex thrLock(mThreadsLock);
    int count = 0;
    // loop through the dispatcher threads
    foreach (shared_ptr<DispatcherThread> &thr, mThreads)
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

bool Forte::OnDemandDispatcher::StopRunningEvent(shared_ptr<Event> &runningEvent)
{
    AutoUnlockMutex lock(mNotifyLock);
    AutoUnlockMutex thrLock(mThreadsLock);

    foreach (shared_ptr<DispatcherThread> &thr, mThreads)
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
