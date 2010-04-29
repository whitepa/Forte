#ifndef _AutoMutex_h
#define _AutoMutex_h
#include <pthread.h>
#include <ctime>
#include <sys/time.h>
#include "Object.h"

namespace Forte
{
    class Mutex : public Object {
        friend class ThreadCondition;
    public:
        inline Mutex(const pthread_mutexattr_t *attr = NULL)
            {
                pthread_mutex_init(&mPThreadMutex, attr);
            }
        inline ~Mutex() {pthread_mutex_destroy(&mPThreadMutex);}
        inline int lock() {return pthread_mutex_lock(&mPThreadMutex);}
        inline int unlock() {return pthread_mutex_unlock(&mPThreadMutex);}
        inline int trylock() {return pthread_mutex_trylock(&mPThreadMutex);}
        inline int timedlock(const struct timespec& timeout)
            {
                struct timespec abs_time;
                if (clock_gettime(CLOCK_REALTIME, &abs_time) != 0)
                {
                    struct timeval now;
                    gettimeofday(&now, NULL);
                    abs_time.tv_sec = now.tv_sec;
                    abs_time.tv_nsec = now.tv_usec * 1000;
                }
                abs_time.tv_sec += timeout.tv_sec;
                abs_time.tv_nsec += timeout.tv_nsec;
                if (abs_time.tv_nsec > 1000000000)
                {
                    abs_time.tv_sec++;
                    abs_time.tv_nsec -= 1000000000;
                }
                return pthread_mutex_timedlock(&mPThreadMutex, &abs_time);
            }
    protected:
        pthread_mutex_t mPThreadMutex;
    };

    class AutoLockMutex : public Object {
    public:
        inline AutoLockMutex(Mutex &mutex):mMutex(mutex) {mMutex.unlock();}
        inline ~AutoLockMutex() {mMutex.lock();}
    private:
        Mutex &mMutex;
    };

    class AutoUnlockMutex : public Object {
    public:
        inline AutoUnlockMutex(Mutex &mutex):mMutex(mutex) {mMutex.lock();}
        inline ~AutoUnlockMutex() {mMutex.unlock();}
    private:
        Mutex &mMutex;
    };

    class AutoUnlockOnlyMutex : public Object {
    public:
        inline AutoUnlockOnlyMutex(Mutex &mutex):mMutex(mutex) { }
        inline ~AutoUnlockOnlyMutex() { mMutex.unlock(); }
    private:
        Mutex &mMutex;
    };
};
#endif
