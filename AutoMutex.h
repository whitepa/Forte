// #SCQAD TEST: ONBOX: AutoMutexOnBoxTest

#ifndef _AutoMutex_h
#define _AutoMutex_h
#include <pthread.h>
#include <ctime>
#include <sys/time.h>
#include "Object.h"

namespace Forte
{
    class Mutex {
        friend class ThreadCondition;


    private:
        // non-copyable:
        // deriving from forte object or boost::non-copyable adds a
        // small but measurable amount of overhead to
        // creation/destruction of this object
        Mutex(Mutex const&);
        Mutex& operator=(Mutex const&);

    public:

        /**
         *Initializes a mutex. While it is possible to specify attributes with attr, Scale 
         *Computing does not use this feature. All initializations have attr set to NULL
         *(same as type PTHREAD_MUTEX_DEFAULT). It is strongly recommended you avoid using a 
         *specified attribute since it has not been tested in the Scale Computing system. 
         *If such usage is absolutely necessary, see the pthread_mutexattr_settype man page.
         *
         *Once a mutex attributes object is initialized, any function affecting the attributes
         *object (including destruction) does not affect any previously initialized mutexes.
         **/
        inline Mutex(const pthread_mutexattr_t *attr = NULL)
            {
                pthread_mutex_init(&mPThreadMutex, attr);
            }

        inline ~Mutex() {pthread_mutex_destroy(&mPThreadMutex);}

        /**
         *Locks a mutex object and blocks access until a lock is acquired. Attempting to recursively lock
         *the mutex results in undefined behavior. 
         **/
        inline int Lock() {return pthread_mutex_lock(&mPThreadMutex);}

        /**
         *If the object is already locked, Unlock() returns with the mutex object referenced in 
         *&mPThreadMutex in the locked state with the calling thread as its owner. Attempting to 
         *unlock a mutex if it was not locked by the calling thread results in undefined behavior. Attempting
         *to unlock the mutex if it is not locked results in undefined behavior.
         **/
        inline int Unlock() {return pthread_mutex_unlock(&mPThreadMutex);}

        /**
         *Attempts to acquire a lock for a mutex object. If the lock referenced by &mPThreadMutex
         *is acquired, it returns zero. Otherwise, an error number is returned to indicate the error.
         **/
        inline int Trylock() {return pthread_mutex_trylock(&mPThreadMutex);}

        /**
         *
         **/
        inline int TimedLock(const struct timespec& timeout)
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

    class RecursiveMutex : public Mutex
    {
    public:
        typedef Mutex base_type;

        RecursiveMutex(const pthread_mutexattr_t *attr = NULL)
            :base_type(NULL)
        {
            pthread_mutexattr_t attrCopy;
            pthread_mutexattr_init(&attrCopy);

            if(attr)
            {
                 attrCopy = *attr;
            }

            pthread_mutexattr_settype(&attrCopy, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&mPThreadMutex, &attrCopy);
        }
    };

    class AutoLockMutex {
    public:
        inline AutoLockMutex(Mutex &mutex):mMutex(mutex) {mMutex.Unlock();}
        inline ~AutoLockMutex() {mMutex.Lock();}

    private:
        // non-copyable: see note above
        AutoLockMutex(AutoLockMutex const&);
        AutoLockMutex& operator=(AutoLockMutex const&);

    private:
        Mutex &mMutex;
    };

    class AutoUnlockMutex {
    public:
        inline AutoUnlockMutex(Mutex &mutex):mMutex(mutex) {mMutex.Lock();}
        inline ~AutoUnlockMutex() {mMutex.Unlock();}

    private:
        // non-copyable: see note above
        AutoUnlockMutex(AutoUnlockMutex const&);
        AutoUnlockMutex& operator=(AutoUnlockMutex const&);

    private:
        Mutex &mMutex;
    };

    class AutoUnlockOnlyMutex {
    public:
        inline AutoUnlockOnlyMutex(Mutex &mutex):mMutex(mutex) { }
        inline ~AutoUnlockOnlyMutex() { mMutex.Unlock(); }

    private:
        // non-copyable: see note above
        AutoUnlockOnlyMutex(AutoUnlockOnlyMutex const&);
        AutoUnlockOnlyMutex& operator=(AutoUnlockOnlyMutex const&);

    private:
        Mutex &mMutex;
    };
};
#endif
