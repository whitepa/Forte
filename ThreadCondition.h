#ifndef _ThreadCondition_h
#define _ThreadCondition_h

#include <pthread.h>
#include "AutoMutex.h"
#include "Clock.h"

namespace Forte
{
    EXCEPTION_CLASS(EThreadCondition);
    class ThreadCondition
    {
    public:
        inline ThreadCondition(Mutex &lock) : mLock(lock)
        {
            if (pthread_condattr_init(&mCondAttribute) == 0)
                if (pthread_condattr_setclock(&mCondAttribute, CLOCK_MONOTONIC) == 0)
                    if (pthread_cond_init(&mCond, &mCondAttribute) == 0)
                        return;
            throw EThreadCondition();
        };
        inline ~ThreadCondition()
        {
            pthread_cond_destroy(&mCond);
            pthread_condattr_destroy(&mCondAttribute);
        };

        inline int Signal() { return pthread_cond_signal(&mCond); };
        inline int Broadcast() { return pthread_cond_broadcast(&mCond); };
        inline int Wait() { return pthread_cond_wait(&mCond, &(mLock.mPThreadMutex)); };
        inline int TimedWait(const struct timespec &abstime)
            { return pthread_cond_timedwait(&mCond, &(mLock.mPThreadMutex), &abstime); };
        inline int TimedWait(const int seconds)
            {
                struct timespec ts;
                MonotonicClock().GetTime(ts);
                ts.tv_sec += seconds;
                return TimedWait(ts);
            }

     protected:
        Mutex &mLock;
        pthread_cond_t mCond;
        pthread_condattr_t mCondAttribute;
    };
};

#endif
