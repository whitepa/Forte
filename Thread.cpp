#include "Forte.h"

// a static pthread_key to store pointers to thread objects
// (for use by static methods)
pthread_key_t CThread::sThreadKey;
pthread_once_t CThread::sThreadKeyOnce;

void CThread::makeKey(void)
{
    pthread_key_create(&sThreadKey, NULL);
}
CThread * CThread::myThread(void)
{
    CThread *thr = reinterpret_cast<CThread*>(pthread_getspecific(sThreadKey));
    if (thr == NULL)
        throw CForteThreadException("CThread::myThread() called from unknown thread");
    return thr;
}
void * CThread::startThread(void *obj)
{ 
    // initialize the thread key once
    pthread_once(&sThreadKeyOnce, makeKey);
    // store pointer to this object in the key
    pthread_setspecific(sThreadKey, obj);

    void *retval = NULL;
    CThread *thr = (CThread *)obj;
    // wait until the thread has been fully initialized
    {
        CAutoUnlockMutex lock(thr->mInitializedLock);
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
    bool selfDelete = thr->mSelfDelete;
    // inform the log manager of this thread
    CLogThreadInfo logThread(CServerMain::GetServer().mLogManager, *thr);
    thr->mThreadName.Format("unknown-%u", (unsigned)thr->mThread);
    if (!thr->mThreadShutdown)
        hlog(HLOG_DEBUG, "%s thread initialized", (selfDelete)?"self deleting":"managed");
    
    // run the thread
    try
    {
        if (!thr->mThreadShutdown) retval = thr->run();
    }
    catch (CException &e)
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

    if (selfDelete)
    {
        hlog(HLOG_DEBUG, "thread is self deleting");
        delete thr;
    }
    else
    {
        // notify that shutdown is complete
        hlog(HLOG_DEBUG, "broadcasting thread shutdown");
        CAutoUnlockMutex lock(thr->mShutdownNotifyLock);
        thr->mShutdownComplete = true;
        thr->mShutdownNotify.broadcast();
    }
    return retval;
}

void CThread::initialized()
{
    CAutoUnlockMutex lock(mInitializedLock);
    mInitialized = true;
    mInitializedNotify.broadcast();
}

void CThread::waitForShutdown()
{
    CAutoUnlockMutex lock(mShutdownNotifyLock);
    if (mShutdownComplete) return;
    mShutdownNotify.wait();
}

CThread::~CThread()
{
    if (mSelfDelete && !mThreadShutdown)
    {
        hlog(HLOG_ERR, "you probably don't want to delete() a self deleting thread; "
             "this may cause the process to crash!  Call shutdown() instead!");
    }
    // make sure the shutdown flag is set
    mThreadShutdown = true;

    if (mSelfDelete)
        pthread_detach(mThread);
    else
        // join the pthread
        // (this will block until the thread exits)
        pthread_join(mThread, NULL);
}
