#include "Forte.h"

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
        throw EThread("Thread::myThread() called from unknown thread");
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
            thr->mInitializedNotify.TimedWait(timeout);
        }
    }
    // inform the log manager of this thread
    LogThreadInfo logThread(LogManager::GetInstance(), *thr);
    thr->mThreadName.Format("unknown-%u", (unsigned)thr->mThread);
    if (!thr->mThreadShutdown)
        hlog(HLOG_DEBUG, "thread initialized");
    
    // run the thread
    try
    {
        if (!thr->mThreadShutdown) retval = thr->run();
    }
    catch (EThreadShutdown &e)
    {
        // normal condition from here on out
    }
    catch (Exception &e)
    {
        hlog(HLOG_ERR, "exception in thread run(): %s",
             e.GetDescription().c_str());
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
    hlog(HLOG_DEBUG, "Broadcasting thread shutdown");
    AutoUnlockMutex lock(thr->mShutdownCompleteLock);
    thr->mShutdownComplete = true;
    thr->mShutdownCompleteCondition.Broadcast();

    return retval;
}

void Thread::initialized()
{
    AutoUnlockMutex lock(mInitializedLock);
    mInitialized = true;
    mInitializedNotify.Broadcast();
}

void Thread::WaitForShutdown()
{
    AutoUnlockMutex lock(mShutdownCompleteLock);
    if (mShutdownComplete) return;
    mShutdownCompleteCondition.Wait();
}

void Thread::interruptibleSleep(const struct timespec &interval, bool throwRequested)
{
    // Sleep for the desired interval.  If the thread is shutdown
    // during that interval, we will return early (or throw
    // EThreadShutdown if so requested).

    // Pthread condition timedwaits use real time clock values, so we
    // have to dance around a bit to get this to work correctly in all
    // cases.
    RealtimeClock rtc;
    MonotonicClock mc;
    struct timespec start = mc.GetTime(); // monotonic start time
    struct timespec now = rtc.GetTime();  // realtime now
    struct timespec end = now + interval; // realtime end of sleep

    AutoUnlockMutex lock(mShutdownRequestedLock);
    while (!mThreadShutdown)
    {
        int status = mShutdownRequested.TimedWait(end);
        if (status == 0)
        {
            // The condition was signalled.
            if (throwRequested)
                throw EThreadShutdown();
            else
                return;
        }
        else if (status == EINVAL || status == EPERM)
            throw EThread(FStringFC(), "Software error: %s", strerror(errno));
        else if (status == ETIMEDOUT)
        {
            // re-evaluate timeout, as spurious wakeups may occur
            AutoLockMutex unlock(mShutdownRequestedLock);
            // mutex is unlocked here
            struct timespec mnow = mc.GetTime(); // monotonic now
            if (interval < mnow - start)
                // yes, we timed out
                return;
            else
            {
                // didn't time out, reset the end timespec and keep waiting
                now = rtc.GetTime();
                end = now + interval - (mnow - start);
            }
        }
        // mutex is re-locked here
    }
}

Thread::~Thread()
{
    // tell the thread to shut down
    Shutdown();

    // Join the pthread
    // (this will block until the thread exits)
    pthread_join(mThread, NULL);
}
