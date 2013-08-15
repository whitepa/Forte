#ifndef _ThreadCondition_h
#define _ThreadCondition_h

// POSIX
#include <pthread.h>

// Forte
#include "AutoMutex.h"
#include "Clock.h"


namespace Forte
{
    EXCEPTION_CLASS(EThreadCondition);
    class ThreadCondition
    {
    public:
        ThreadCondition(Mutex &lock);

        ~ThreadCondition();

        inline int Signal() {
            return pthread_cond_signal(&mCond);
        }

        inline int Broadcast() {
            return pthread_cond_broadcast(&mCond);
        }

        inline int Wait() {
            return pthread_cond_wait(&mCond, &(mLock.mPThreadMutex));
        }

        inline int TimedWait(const struct timespec &abstime)  {
            return pthread_cond_timedwait(&mCond,
                                          &(mLock.mPThreadMutex),
                                          &abstime);
        }

        int TimedWait(int seconds);

     protected:
        Mutex &mLock;
        pthread_cond_t mCond;
    };
};

#endif
