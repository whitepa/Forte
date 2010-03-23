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
                pthread_mutex_init(&m_pthread_mutex, attr);
            }
        inline ~Mutex() {pthread_mutex_destroy(&m_pthread_mutex);}
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

    class AutoLockMutex : public Object {
    public:
        inline AutoLockMutex(Mutex &mutex):m_mutex(mutex) {m_mutex.unlock();}
        inline ~AutoLockMutex() {m_mutex.lock();}
    private:
        Mutex &m_mutex;
    };

    class AutoUnlockMutex : public Object {
    public:
        inline AutoUnlockMutex(Mutex &mutex):m_mutex(mutex) {m_mutex.lock();}
        inline ~AutoUnlockMutex() {m_mutex.unlock();}
    private:
        Mutex &m_mutex;
    };

    class AutoUnlockOnlyMutex : public Object {
    public:
        inline AutoUnlockOnlyMutex(Mutex &mutex):m_mutex(mutex) { }
        inline ~AutoUnlockOnlyMutex() { m_mutex.unlock(); }
    private:
        Mutex &m_mutex;
    };
};
#endif
