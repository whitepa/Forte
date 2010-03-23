#include "Forte.h"

// a static pthread_key to store pointers to thread objects
// (for use by static methods)
pthread_key_t Thread::sThreadKey;
pthread_once_t Thread::sThreadKeyOnce;

void Thread::makeKey(void)
{
    pthread_key_create(&sThreadKey, NULL);
}
Thread * Thread::myThread(void)
{
    Thread *thr = reinterpret_cast<Thread*>(pthread_getspecific(sThreadKey));
    if (thr == NULL)
        throw ForteThreadException("Thread::myThread() called from unknown thread");
    return thr;
}
void * Thread::startThread(void *obj)
{ 
    // initialize the thread key once
    pthread_once(&sThreadKeyOnce, makeKey);
    // store pointer to this object in the key
    pthread_setspecific(sThreadKey, obj);

    void *retval = NULL;
    Thread *thr = (Thread *)obj;
    // wait until the thread has been fully initialized
    {
        AutoUnlockMutex lock(thr->mInitializedLock);
        while(!thr->mInitialized && !thr->mThreadShutdown)
        {
            struct timeval now;
            struct timespec timeout;
            gettimeofday(&now, 0);
            timeout.tv_sec = now.tv_sec + 1; // wake up every second to check for shutdown
            timeout.tv_nsec = now.tv_usec * 1000;
            thr->mInitializedNotify.timedwait(timeout);
        }
    }
    // inform the log manager of this thread
    LogThreadInfo logThread(ServerMain::GetServer().mLogManager, *thr);
    thr->mThreadName.Format("unknown-%u", (unsigned)thr->mThread);
    if (!thr->mThreadShutdown)
        hlog(HLOG_DEBUG, "thread initialized");
    
    // run the thread
    try
    {
        if (!thr->mThreadShutdown) retval = thr->run();
    }
    catch (Exception &e)
    {
        hlog(HLOG_ERR, "exception in thread run(): %s",
             e.getDescription().c_str());
    }
    catch (std::exception &e)
    {
        hlog(HLOG_ERR, "exception in thread run(): %s",
             e.what());
    }
    catch (...)
    {
        hlog(HLOG_ERR, "unknown exception in thread run()");
    }
    
    hlog(HLOG_DEBUG, "thread shutting down");
    thr->mThreadShutdown = true;

    // notify that shutdown is complete
    hlog(HLOG_DEBUG, "broadcasting thread shutdown");
    AutoUnlockMutex lock(thr->mShutdownNotifyLock);
    thr->mShutdownComplete = true;
    thr->mShutdownNotify.broadcast();

    return retval;
}

void Thread::initialized()
{
    AutoUnlockMutex lock(mInitializedLock);
    mInitialized = true;
    mInitializedNotify.broadcast();
}

void Thread::waitForShutdown()
{
    AutoUnlockMutex lock(mShutdownNotifyLock);
    if (mShutdownComplete) return;
    mShutdownNotify.wait();
}

Thread::~Thread()
{
    // make sure the shutdown flag is set
    mThreadShutdown = true;

    // join the pthread
    // (this will block until the thread exits)
    pthread_join(mThread, NULL);
}
