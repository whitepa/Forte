#include "OnDemandDispatcher.h"
#include "LogManager.h"
#include "Foreach.h"

using namespace Forte;

/////////////////////////// On demand dispatcher

Forte::OnDemandDispatcherManager::OnDemandDispatcherManager(OnDemandDispatcher &disp) :
    DispatcherThread(disp) 
{
    initialized();
}

void * Forte::OnDemandDispatcherManager::run(void)
{
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
            for (i = disp.mThreads.begin(); 
                 i != disp.mThreads.end(); ++i)
            {
                // OnDemandDispatcher workers are guaranteed to be
                // finished if HasEvent() returns false.
                if (!(*i)->HasEvent())
                    disp.mThreads.erase(i);
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
        }
        if (disp.mShutdown) break;

        struct timeval now;
        struct timespec timeout;
        gettimeofday(&now, 0);
        timeout.tv_sec = now.tv_sec + 1; // wake up every second
        timeout.tv_nsec = now.tv_usec * 1000;
        disp.mNotify.TimedWait(timeout);
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
    hlog(HLOG_DEBUG, "dispatcher shutdown complete");
    return NULL;
}
Forte::OnDemandDispatcherWorker::OnDemandDispatcherWorker(OnDemandDispatcher &disp, 
                                                          shared_ptr<Event> event) :
    DispatcherThread(disp)
{
    mEventPtr = event;
    initialized();
}
Forte::OnDemandDispatcherWorker::~OnDemandDispatcherWorker()
{
    OnDemandDispatcher &disp(dynamic_cast<OnDemandDispatcher&>(mDispatcher));
    disp.mRequestHandler->Cleanup();
    disp.mThreadSem.Post();
}

void * Forte::OnDemandDispatcherWorker::run()
{
    OnDemandDispatcher &disp(dynamic_cast<OnDemandDispatcher&>(mDispatcher));
    mThreadName.Format("%s-od-%u", disp.mDispatcherName.c_str(), GetThreadID());
    disp.mRequestHandler->Init();
    disp.mRequestHandler->Handler(mEventPtr.get(), this);
    // thread is complete at this point, resetting our event pointer
    // will cause the manager to reap us
    mEventPtr.reset();
    return NULL;
}

Forte::OnDemandDispatcher::OnDemandDispatcher(boost::shared_ptr<RequestHandler> requestHandler,
                                              const int maxThreads,
                                              const int deepQueue, const int maxDepth, 
                                              const char *name):
    Dispatcher(requestHandler, maxDepth, name),
    mMaxThreads(maxThreads),
    mThreadSem(maxThreads),
    mManagerThread(*this)
{
    mDispatcherName = name;
}
Forte::OnDemandDispatcher::~OnDemandDispatcher()
{
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
void Forte::OnDemandDispatcher::Pause(void) { mPaused = 1; }
void Forte::OnDemandDispatcher::Resume(void) { mPaused = 0; mNotify.Signal(); }
void Forte::OnDemandDispatcher::Enqueue(shared_ptr<Event> e) { mEventQueue.Add(e); }
bool Forte::OnDemandDispatcher::Accepting(void) { return mEventQueue.Accepting(); }
int Forte::OnDemandDispatcher::GetRunningEvents(int maxEvents,
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
