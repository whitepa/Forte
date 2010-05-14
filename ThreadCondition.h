#ifndef _ThreadCondition_h
#define _ThreadCondition_h

#include <pthread.h>
#include "AutoMutex.h"
namespace Forte
{
    class ThreadCondition
    {
    public:
        inline ThreadCondition(Mutex &lock) : mLock(lock) { pthread_cond_init(&mCond, NULL); };
        inline ~ThreadCondition() { pthread_cond_destroy(&mCond); };
    
        inline int Signal() { return pthread_cond_signal(&mCond); };
        inline int Broadcast() { return pthread_cond_broadcast(&mCond); };
        inline int Wait() { return pthread_cond_wait(&mCond, &(mLock.mPThreadMutex)); };
        inline int TimedWait(const struct timespec &abstime)
            { return pthread_cond_timedwait(&mCond, &(mLock.mPThreadMutex), &abstime); };
        inline int TimedWait(const int seconds)
            {
                struct timeval tv;
                struct timespec ts;
                gettimeofday(&tv, NULL);
                ts.tv_sec = tv.tv_sec + seconds;
                ts.tv_nsec = tv.tv_usec * 1000;
                return TimedWait(ts);
            }
    
     protected:
        Mutex &mLock;
        pthread_cond_t mCond;
    };
};

#endif
