#ifndef __Thread_h
#define __Thread_h

#include <pthread.h>
#include "FString.h"
#include "Object.h"
#include "Semaphore.h"
#include "Exception.h"

// A generic thread class.  The user should derive from this class and define
// the run() method.

// Upon instantiation, a pthread will be created, but it will not call run()
// until the value of mInitialized becomes true.  This allows the use of 
// initialization lists in a subclass.  Calling initialized() in the body
// of the subclass constructor will start the thread.

// XXX create a static structure to track all threads

namespace Forte
{
    EXCEPTION_CLASS(EThread);
    EXCEPTION_SUBCLASS2(EThread, EThreadShutdown, "Thread Shutting Down");

    class Thread : public Object
    {
    public:
        inline Thread(void) : 
            mInitializedNotify(mInitializedLock), 
            mInitialized(false),
            mThreadShutdown(false), 
            mShutdownRequested(mShutdownRequestedLock),
            mShutdownComplete(false),
            mShutdownCompleteCondition(mShutdownCompleteLock)
            { pthread_create(&mThread, NULL, Thread::startThread, this); };
        virtual ~Thread();

        // Tell a thread to shut itself down.
        inline void shutdown(void) { 
            mThreadShutdown = true; 
            AutoUnlockMutex lock(mShutdownRequestedLock);
            mShutdownRequested.signal();
        };

        // Wait for a thread to shutdown.
        void waitForShutdown(void);

        // is a thread flagged to be shut down?
        bool isShuttingDown(void) { return mThreadShutdown; }

    private:
        // static callback for pthread_once
        static void makeKey(void);
    public:
        // get pointer to your thread object
        static Thread * myThread(void);

        unsigned int mPid;
        FString mThreadName;
    protected:
        void initialized(void);
        virtual void *run(void) = 0;
        static void *startThread(void *obj);

        /**
         * interruptibleSleep will sleep until the given interval
         * elapses, or until the thread is requested to shut down,
         * which ever happens first.
         */
        void interruptibleSleep(const struct timespec &interval,
                                bool throwRequested = true);

        pthread_t mThread;

        Mutex mInitializedLock;
        ThreadCondition mInitializedNotify;
        bool mInitialized;
        bool mThreadShutdown;
        Mutex mShutdownRequestedLock;
        ThreadCondition mShutdownRequested;
        bool mShutdownComplete;
        Mutex mShutdownCompleteLock;
        ThreadCondition mShutdownCompleteCondition;
        static pthread_key_t sThreadKey;
        static pthread_once_t sThreadKeyOnce;
    };
};
#endif
