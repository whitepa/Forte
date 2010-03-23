#include "OnDemandDispatcher.h"
#include "LogManager.h"

/////////////////////////// On demand dispatcher

void * Forte::OnDemandDispatcherManager::run(void)
{
    OnDemandDispatcher *disp = dynamic_cast<OnDemandDispatcher*>(mDispatcher);
    mThreadName.Format("dsp-od-%u", (unsigned)mThread);
    
    AutoUnlockMutex lock(disp->mNotifyLock);
    while (1) // always check for queued events before shutting down
    {
        Event *event;
        while (!disp->mPaused && (event = disp->mEventQueue.get()))
        {
            AutoLockMutex unlock(disp->mNotifyLock);
            disp->mThreadSem.wait();
            new OnDemandDispatcherWorker(disp, event);
        }
        if (disp->mShutdown) break;

        struct timeval now;
        struct timespec timeout;
        gettimeofday(&now, 0);
        timeout.tv_sec = now.tv_sec + 1; // wake up every second
        timeout.tv_nsec = now.tv_usec * 1000;
        disp->mNotify.timedwait(timeout);
    }

    // signal any workers to shutdown
    {
        std::vector<DispatcherThread*>::iterator i;
        AutoUnlockMutex thrlock(disp->mThreadsLock);
        for (i = disp->mThreads.begin(); i != disp->mThreads.end(); ++i)
        {
            DispatcherThread *thr = *i;
            if (thr != NULL) thr->shutdown();
        }
    }    
    // wait for workers to exit
    // wait for the thread vector to empty itself
    while (1)
    {
        {
            AutoUnlockMutex thrlock(disp->mThreadsLock);
            if (disp->mThreads.size() == 0)
                break;
        }
        usleep(250000); // check this reasonably often to avoid shutdown delays
    }
    hlog(HLOG_DEBUG, "dispatcher shutdown complete");
    return NULL;
}
Forte::OnDemandDispatcherWorker::OnDemandDispatcherWorker(OnDemandDispatcher *disp, Event *event) :
    mEvent(event)
{
//    mSelfDelete = true; // workers should self-delete
    mDispatcher = (Dispatcher*)disp;
    disp->registerThread(this);
    initialized();
}
Forte::OnDemandDispatcherWorker::~OnDemandDispatcherWorker()
{
    OnDemandDispatcher *disp = dynamic_cast<OnDemandDispatcher*>(mDispatcher);
    disp->mRequestHandler.cleanup();
    disp->unregisterThread(this);
    disp->mThreadSem.post();
}

void * Forte::OnDemandDispatcherWorker::run()
{
    OnDemandDispatcher *disp = dynamic_cast<OnDemandDispatcher*>(mDispatcher);
    mThreadName.Format("%s-od-%u", mDispatcher->mDispatcherName.c_str(), (unsigned)mThread);
    disp->mRequestHandler.init();
    disp->mRequestHandler.handler(mEvent.get());
    return NULL;
}

Forte::OnDemandDispatcher::OnDemandDispatcher(RequestHandler &requestHandler,
                                              const int maxThreads,
                                              const int deepQueue, const int maxDepth, const char *name):
    mRequestHandler(requestHandler),
    mMaxThreads(maxThreads),
    mThreadSem(maxThreads),
    mNotify(mNotifyLock),
    mEventQueue(maxDepth, &mNotify),
    mManagerThread(this)
{
    mDispatcherName = name;
}
Forte::OnDemandDispatcher::~OnDemandDispatcher()
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
void Forte::OnDemandDispatcher::pause(void) { mPaused = 1; }
void Forte::OnDemandDispatcher::resume(void) { mPaused = 0; mNotify.signal(); }
void Forte::OnDemandDispatcher::enqueue(Event *e) { mEventQueue.add(e); }
bool Forte::OnDemandDispatcher::accepting(void) { return mEventQueue.accepting(); }
int Forte::OnDemandDispatcher::getRunningEvents(int maxEvents,
                                                std::list<Event*> &runningEvents)
{
    // lock the notify lock to prevent threads from freeing events while
    // we are copying them
    AutoUnlockMutex lock(mNotifyLock);
    std::vector<DispatcherThread*>::iterator i;
    int count = 0;
    // loop through the dispatcher threads
    for (i = mThreads.begin(); i != mThreads.end(); ++i)
    {
        DispatcherThread *dt = *i;
        // copy each event
        if (dt != NULL && dt->mEventPtr != NULL)
        {
            runningEvents.push_back(dt->mEventPtr->copy());
            ++count;
        }
    }
    return count;
}

