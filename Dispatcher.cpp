#include "Forte.h"

Dispatcher::Dispatcher() :
    mPaused(false),
    mShutdown(false)
{}
Dispatcher::~Dispatcher()
{
    // XXX
}

void Dispatcher::registerThread(DispatcherThread *thr)
{
    AutoUnlockMutex lock(mThreadsLock); 
    mThreads.push_back(thr);
}
void Dispatcher::unregisterThread(DispatcherThread *thr)
{
    hlog(HLOG_DEBUG, "unregisterThread()");
    AutoUnlockMutex lock(mThreadsLock);
    std::vector<DispatcherThread*>::iterator i;
    for (i = mThreads.begin(); i!=mThreads.end(); ++i)
    {
        if (*i == thr)
        {
            mThreads.erase(i);
            break;
        }
    }
}
//DispatcherThread::DispatcherThread() :
//    Thread(true) // use self deleting threads
//{}
//    // detach ourselves, since no one will be joining us
//    pthread_detach(mThread);
//}
DispatcherThread::DispatcherThread() :
    mEventPtr(NULL)
{}
DispatcherThread::~DispatcherThread()
{
    // XXX
}
////////////////////////////// Thread pool dispatcher
void * ThreadPoolDispatcherManager::run(void)
{
    ThreadPoolDispatcher *disp = dynamic_cast<ThreadPoolDispatcher*>(mDispatcher);
    mThreadName.Format("%s-disp-%u", disp->mDispatcherName.c_str(), (unsigned)mThread);
    
    // start initial worker threads
    for (unsigned int i = 0; i < disp->mMinThreads; ++i)
    {
        disp->mThreadSem.wait();
        try
        {
            new ThreadPoolDispatcherWorker(disp);
        }
        catch (...)
        {
            // XXX log
            disp->mThreadSem.post();
            break;
        }
    }
    // loop until shutdown
    int lastNew = 0;
    while (!disp->mShutdown)
    {
        // see if new threads are needed
        sleep(1);
        int currentThreads = disp->mMaxThreads - disp->mThreadSem.getvalue();
        int spareThreads = disp->mSpareThreadSem.getvalue();
        int newThreadsNeeded = disp->mEventQueue.depth() + disp->mMinSpareThreads - spareThreads;
        int numNew = 0;
        if (newThreadsNeeded < (int)disp->mMinThreads - currentThreads)
            newThreadsNeeded = (int)disp->mMinThreads - currentThreads;
//        hlog(HLOG_DEBUG4, "ThreadPool Manager Loop: %d threads; %d spare; %d needed",
//             currentThreads, spareThreads, newThreadsNeeded);
        if (!disp->mShutdown && newThreadsNeeded > 0)
        {
            numNew = (lastNew == 0) ? 1 : lastNew * 2;
            // we must now create numNew threads
            for (int i = 0; i < numNew; ++i)
            {
                if (disp->mThreadSem.trywait() == -1 && errno == EAGAIN)
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
                        disp->mThreadSem.post();
                        numNew = 0;
                        break;
                    }
                }
            }
        }
        else if (spareThreads > (int)disp->mMaxSpareThreads &&
                 spareThreads > (int)disp->mMinSpareThreads)
        {
            // too many spare threads, shut one down
//            hlog(HLOG_DEBUG, "shutting down a thread");
            AutoUnlockMutex lock(disp->mThreadsLock);
            std::vector<DispatcherThread*>::iterator i = disp->mThreads.begin();
            (*i)->shutdown();
        }
        lastNew = (numNew > 0) ? numNew : 0;
    }

    // shut down
    hlog(HLOG_DEBUG, "'%s' dispatcher shutting down, handling final queued requests...",
         disp->mDispatcherName.c_str());

    // wait until all requests are processed
    disp->mEventQueue.waitUntilEmpty();
    
    hlog(HLOG_DEBUG, "'%s' dispatcher shutting down, waiting for threads to exit...",
         disp->mDispatcherName.c_str());

    // tell all workers to shutdown
    std::vector<DispatcherThread*>::iterator ti;
    {
        AutoUnlockMutex lock(disp->mThreadsLock);
        for (ti = disp->mThreads.begin(); ti != disp->mThreads.end(); ti++)
            (*ti)->shutdown();
        // when the threads shutdown, they will delete themselves, and remove
        // themselves from the vector
    }
    // wait for the thread vector to empty itself
    int waitcount = 0;
    while (1)
    {
        if (disp->mThreads.empty())
            break;
        if (++waitcount > 240)
        {
            // one minute has gone by, stop waiting
            hlog(HLOG_CRIT, "timeout waiting for thread shutdown!");
            break;
        }
        usleep(250000); // check this reasonably often to avoid shutdown delays
    }
    hlog(HLOG_DEBUG, "'%s' dispatcher shutdown complete", disp->mDispatcherName.c_str());
    return NULL;
}

ThreadPoolDispatcherWorker::ThreadPoolDispatcherWorker(ThreadPoolDispatcher *disp) :
    mLastPeriodicCall(time(0))
{
//    mSelfDelete = true; // workers should self-delete
    mDispatcher = (Dispatcher*)disp;
    disp->registerThread(this);
    initialized();
}
ThreadPoolDispatcherWorker::~ThreadPoolDispatcherWorker()
{
    ThreadPoolDispatcher *disp = dynamic_cast<ThreadPoolDispatcher*>(mDispatcher);
    assert(disp != NULL);
    disp->mRequestHandler.cleanup();
    disp->unregisterThread(this);
    disp->mThreadSem.post();
}
void * ThreadPoolDispatcherWorker::run(void)
{
    ThreadPoolDispatcher *disp = dynamic_cast<ThreadPoolDispatcher*>(mDispatcher);
    mThreadName.Format("%s-pool-%u", mDispatcher->mDispatcherName.c_str(), (unsigned)mThread);
    
    // call the request handler's initialization hook
    disp->mRequestHandler.init();

    // pull events from the request queue
    while (!mThreadShutdown)
    {
        AutoUnlockMutex lock(disp->mNotifyLock);
        while (disp->mPaused == false &&
               !mThreadShutdown)
        {
            auto_ptr<Event> event(disp->mEventQueue.get());
            if (event.get() == NULL) break;
            // set event pointer here
            mEventPtr = event.get();
            // set start time
            gettimeofday(&(mEventPtr->mStartTime), NULL);
            {
                AutoLockMutex unlock(disp->mNotifyLock);
                // process the request
                try
                {
                    disp->mRequestHandler.handler(event.get());
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
            mEventPtr = NULL;
            // event will be deleted HERE
        }
        struct timeval now;
        struct timespec timeout;
        gettimeofday(&now, 0);
        timeout.tv_sec = now.tv_sec + 1; // wake up every second
        timeout.tv_nsec = now.tv_usec * 1000;
        disp->mSpareThreadSem.post();
        disp->mNotify.timedwait(timeout);
        disp->mSpareThreadSem.trywait();
        if (mThreadShutdown) break;
        // see if we need to run periodic
        gettimeofday(&now, 0);
        if (disp->mRequestHandler.mTimeout != 0 &&
            (unsigned int)(now.tv_sec - mLastPeriodicCall) > disp->mRequestHandler.mTimeout)
        {
            disp->mRequestHandler.periodic();
            mLastPeriodicCall = now.tv_sec;
        }
    }
    return NULL;
}

ThreadPoolDispatcher::ThreadPoolDispatcher(RequestHandler &requestHandler,
                                             const int minThreads, const int maxThreads, 
                                             const int minSpareThreads, const int maxSpareThreads,
                                             const int deepQueue, const int maxDepth, const char *name):
    mRequestHandler(requestHandler),
    mMinThreads(minThreads),
    mMaxThreads(maxThreads),
    mMinSpareThreads(minSpareThreads),
    mMaxSpareThreads(maxSpareThreads),
    mThreadSem(maxThreads),
    mSpareThreadSem(0),
    mNotify(mNotifyLock),
    mEventQueue(maxDepth, &mNotify),
    mManagerThread(this)
{
    mDispatcherName = name;
}
ThreadPoolDispatcher::~ThreadPoolDispatcher()
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
void ThreadPoolDispatcher::pause(void) { mPaused = 1; }
void ThreadPoolDispatcher::resume(void) { mPaused = 0; mNotify.broadcast(); }
void ThreadPoolDispatcher::enqueue(Event *e)
{ 
    if (mShutdown)
        throw ForteThreadPoolDispatcherException("dispatcher is shutting down; no new events are being accepted");
    mEventQueue.add(e); 
}
bool ThreadPoolDispatcher::accepting(void) { return mEventQueue.accepting(); }

int ThreadPoolDispatcher::getRunningEvents(int maxEvents,
                                            std::list<Event*> &runningEvents)
{
    // loop through the dispatcher threads
    AutoUnlockMutex lock(mNotifyLock);
    std::vector<DispatcherThread*>::iterator i;
    int count = 0;
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

/////////////////////////// On demand dispatcher

void * OnDemandDispatcherManager::run(void)
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
OnDemandDispatcherWorker::OnDemandDispatcherWorker(OnDemandDispatcher *disp, Event *event) :
    mEvent(event)
{
//    mSelfDelete = true; // workers should self-delete
    mDispatcher = (Dispatcher*)disp;
    disp->registerThread(this);
    initialized();
}
OnDemandDispatcherWorker::~OnDemandDispatcherWorker()
{
    OnDemandDispatcher *disp = dynamic_cast<OnDemandDispatcher*>(mDispatcher);
    disp->mRequestHandler.cleanup();
    disp->unregisterThread(this);
    disp->mThreadSem.post();
}

void * OnDemandDispatcherWorker::run()
{
    OnDemandDispatcher *disp = dynamic_cast<OnDemandDispatcher*>(mDispatcher);
    mThreadName.Format("%s-od-%u", mDispatcher->mDispatcherName.c_str(), (unsigned)mThread);
    disp->mRequestHandler.init();
    disp->mRequestHandler.handler(mEvent.get());
    return NULL;
}

OnDemandDispatcher::OnDemandDispatcher(RequestHandler &requestHandler,
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
OnDemandDispatcher::~OnDemandDispatcher()
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
void OnDemandDispatcher::pause(void) { mPaused = 1; }
void OnDemandDispatcher::resume(void) { mPaused = 0; mNotify.signal(); }
void OnDemandDispatcher::enqueue(Event *e) { mEventQueue.add(e); }
bool OnDemandDispatcher::accepting(void) { return mEventQueue.accepting(); }
int OnDemandDispatcher::getRunningEvents(int maxEvents,
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

