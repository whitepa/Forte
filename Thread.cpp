#include "Thread.h"
#include "FTrace.h"
#include "LogManager.h"
#include "Clock.h"
#include "Util.h"

using namespace Forte;

// a static pthread_key to store pointers to thread objects
// (for use by static methods)
pthread_key_t Thread::sThreadKey;
pthread_once_t Thread::sThreadKeyOnce;

void Thread::makeKey(void)
{
    pthread_key_create(&sThreadKey, NULL);
}
Thread * Thread::MyThread(void)
{
    Thread *thr = reinterpret_cast<Thread*>(pthread_getspecific(sThreadKey));
    if (thr == NULL)
        throw EThreadUnknown();
    return thr;
}
void * Thread::startThread(void *obj)
{ 
    FTRACE;

    // initialize the thread key once
    pthread_once(&sThreadKeyOnce, makeKey);
    // store pointer to this object in the key
    pthread_setspecific(sThreadKey, obj);

    void *retval = NULL;
    Thread *thr = (Thread *)obj;
    // wait until the thread has been fully initialized
    {
        AutoUnlockMutex lock(thr->mNotifyLock);
        while(!thr->mInitialized && !thr->mThreadShutdown)
            thr->mNotifyCond.Wait();
    }
    // inform the log manager of this thread
    LogThreadInfo logThread(*thr);
    if (thr->mThreadName.empty())
        thr->mThreadName.Format("unknown-%u", (unsigned)thr->mThread);
    if (!thr->mThreadShutdown)
        hlog(HLOG_DEBUG2, "thread initialized");
    
    // run the thread
    try
    {
        if (!thr->mThreadShutdown) retval = thr->run();
    }
    catch (EThreadShutdown &e)
    {
        // normal condition from here on out
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
    
    hlog(HLOG_DEBUG2, "thread shutting down");
    thr->mThreadShutdown = true;

    // notify that shutdown is complete
    AutoUnlockMutex lock(thr->mShutdownCompleteLock);
    thr->mShutdownComplete = true;
    hlog(HLOG_DEBUG2, "Broadcasting thread shutdown");
    thr->mShutdownCompleteCondition.Broadcast();

    return retval;
}

void Thread::initialized()
{
    AutoUnlockMutex lock(mNotifyLock);
    mInitialized = true;
    mNotifyCond.Broadcast();
}

void Thread::deleting()
{
    if (mDeletingCalled)
    {
        hlog(HLOG_ERR, "deleting() called more than once");
        return;
    }
    // tell the thread to shut down
    Shutdown();
    // Join the pthread
    // (this will block until the thread exits)
    pthread_join(mThread, NULL);
    mDeletingCalled = true;
}

void Thread::WaitForShutdown()
{
    AutoUnlockMutex lock(mShutdownCompleteLock);
    if (mShutdownComplete) return;
    while (!mShutdownComplete)
        mShutdownCompleteCondition.Wait();
}

void Thread::WaitForInitialize()
{
    FTRACE;
    AutoUnlockMutex lock(mNotifyLock);
    while (!mInitialized)
    {
        mNotifyCond.Wait();
    }
}
void Thread::InterruptibleSleep(const struct timespec &interval,
                                bool throwOnShutdown)
{
    // (static version)
    Thread *thr = MyThread();
    thr->interruptibleSleep(interval, throwOnShutdown);
}

void Thread::interruptibleSleep(const struct timespec &interval, bool throwOnShutdown)
{
    // Sleep for the desired interval.  If the thread is shutdown or notified
    // during that interval, we will return early (or throw
    // EThreadShutdown if so requested).

    // Forte::ThreadConditions use the monotonic clock now, but we still need to
    // dance around spurious wakeups

    MonotonicClock mc;
    struct timespec start = mc.GetTime(); // monotonic start time
    struct timespec end = start + interval; // monotonic end of sleep

    AutoUnlockMutex lock(mNotifyLock);
    while (!mThreadShutdown && !mNotified)
    {
        int status = mNotifyCond.TimedWait(end);
        if (status == 0)
        {
            // The condition was signalled.
            if (mThreadShutdown && throwOnShutdown)
                throw EThreadShutdown();
            else
                return;
        }
        else if (status == EINVAL || status == EPERM)
            throw EThread(FStringFC(), "Software error: %s", strerror(errno));
        else if (status == ETIMEDOUT)
        {
            // evaluate timeout actually occurred, as spurious wakeups may occur
            AutoLockMutex unlock(mNotifyLock);
            // mutex is unlocked here
            struct timespec mnow = mc.GetTime(); // monotonic now
            if (interval < mnow - start)
                return; // yes, we timed out
        }
        // mutex is re-locked here
    }
    mNotified = false;
}

Thread::~Thread()
{
    FTRACE;
    if (!mDeletingCalled)
    {
        hlog(HLOG_CRIT, "Software error: "
             "dtor failed to call deleting() in thread '%s'",
             mThreadName.c_str());
        deleting();
    }
}

void Thread::Shutdown(void)
{
    FTRACE;
    mThreadShutdown = true;
    Notify();
}
