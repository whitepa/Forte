#ifndef _AutoMutex_h
#define _AutoMutex_h
#include <pthread.h>
#include <ctime>
#include <sys/time.h>
#include "Object.h"

namespace Forte
{
    class CMutex : public Object {
        friend class CThreadCondition;
    public:
        inline CMutex(const pthread_mutexattr_t *attr = NULL)
            {
                pthread_mutex_init(&m_pthread_mutex, attr);
            }
        inline ~CMutex() {pthread_mutex_destroy(&m_pthread_mutex);}
        inline int lock() {return pthread_mutex_lock(&m_pthread_mutex);}
        inline int unlock() {return pthread_mutex_unlock(&m_pthread_mutex);}
        inline int trylock() {return pthread_mutex_trylock(&m_pthread_mutex);}
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
                return pthread_mutex_timedlock(&m_pthread_mutex, &abs_time);
            }
    protected:
        pthread_mutex_t m_pthread_mutex;
    };

    class CAutoLockMutex : public Object {
    public:
        inline CAutoLockMutex(CMutex &mutex):m_mutex(mutex) {m_mutex.unlock();}
        inline ~CAutoLockMutex() {m_mutex.lock();}
    private:
        CMutex &m_mutex;
    };

    class CAutoUnlockMutex : public Object {
    public:
        inline CAutoUnlockMutex(CMutex &mutex):m_mutex(mutex) {m_mutex.lock();}
        inline ~CAutoUnlockMutex() {m_mutex.unlock();}
    private:
        CMutex &m_mutex;
    };

    class CAutoUnlockOnlyMutex : public Object {
    public:
        inline CAutoUnlockOnlyMutex(CMutex &mutex):m_mutex(mutex) { }
        inline ~CAutoUnlockOnlyMutex() { m_mutex.unlock(); }
    private:
        CMutex &m_mutex;
    };
};
#endif
