#ifndef _ThreadCondition_h
#define _ThreadCondition_h

#include <pthread.h>
#include "AutoMutex.h"

class CThreadCondition
{
 public:
    inline CThreadCondition(CMutex &lock) : mLock(lock) { pthread_cond_init(&mCond, NULL); };
    inline ~CThreadCondition() { pthread_cond_destroy(&mCond); };
    
    inline int signal() { return pthread_cond_signal(&mCond); };
    inline int broadcast() { return pthread_cond_broadcast(&mCond); };
    inline int wait() { return pthread_cond_wait(&mCond, &(mLock.m_pthread_mutex)); };
    inline int timedwait(const struct timespec &abstime)
        { return pthread_cond_timedwait(&mCond, &(mLock.m_pthread_mutex), &abstime); };
    inline int timedwait(const int seconds)
        {
            struct timeval tv;
            struct timespec ts;
            gettimeofday(&tv, NULL);
            ts.tv_sec = tv.tv_sec + seconds;
            ts.tv_nsec = tv.tv_usec * 1000;
            return timedwait(ts);
        }
    
 protected:
    CMutex &mLock;
    pthread_cond_t mCond;
};


#endif
