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
    EXCEPTION_SUBCLASS2(EThread, EThreadUnknown, "Thread::myThread() called from unknown thread");
    EXCEPTION_SUBCLASS2(EThread, EThreadShutdown, "Thread Shutting Down");

    class Thread : virtual public Object
    {
    public:
        inline Thread(void) :
            mInitialized(false),
            mDeletingCalled(false),
            mThreadShutdown(false),
            mNotified(false),
            mNotifyCond(mNotifyLock),
            mShutdownComplete(false),
            mShutdownCompleteCondition(mShutdownCompleteLock)
            {
                pthread_create(&mThread, NULL, Thread::startThread, this);
                mThreadID = static_cast<unsigned int>(mThread);
            }
        virtual ~Thread();

        // Tell a thread to shut itself down.
        virtual void Shutdown(void);

        inline void Notify(void) {
            AutoUnlockMutex lock(mNotifyLock);
            mNotified = true;
            mNotifyCond.Signal();
        }

        // Wait for a thread to shutdown.
        void WaitForShutdown(void);

        // Wait for this tread to be initialized
        void WaitForInitialize(void);

        // is a thread flagged to be shut down?
        bool IsShuttingDown(void) const {
            Forte::AutoUnlockMutex lock(mNotifyLock);
            return mThreadShutdown;
        }

    private:
        // static callback for pthread_once
        static void makeKey(void);

    public:
        // get pointer to your thread object
        static Thread * MyThread(void);

        unsigned int GetThreadID(void) { return mThreadID; }

        /**
         * InterruptibleSleep() is a static version of the same
         * protected interruptibleSleep() functionality, with the
         * exception that this version may be called from any type of
         * object in any thread.  If called from a Forte::Thread, this
         * will do a full interruptible sleep on that thread.  If
         * called from any other thread, EUnknownThread will be
         * thrown.
         */
        static void InterruptibleSleep(const struct timespec &interval,
                                       bool throwOnShutdown = true);

        FString mThreadName;

    protected:
        /**
         * initialized() must be called by the derived class
         * constructor once the object has been fully initialized.
         * This will trigger the creation of the thread itself.
         */
        void initialized(void);
        /**
         * deleting() must be called by the derived class destructor.
         * This will notify the thread to shutdown (via Shutdown())
         * and will block until the thread has exited.
         */
        void deleting(void);

        void setThreadName(const FString &name) { mThreadName = name; }

        virtual void *run(void) = 0;

        /**
         * interruptibleSleep will sleep until the given interval
         * elapses, or until the thread is requested to shut down,
         * which ever happens first.
         */
        void interruptibleSleep(const struct timespec &interval,
                                bool throwRequested = true);

    private:
        static void *startThread(void *obj);

        pthread_t mThread;
        unsigned int mThreadID;

        bool mInitialized;
        bool mDeletingCalled;

        bool mThreadShutdown;

        bool mNotified;

        mutable Mutex mNotifyLock;
        ThreadCondition mNotifyCond;

        bool mShutdownComplete;
        Mutex mShutdownCompleteLock;
        ThreadCondition mShutdownCompleteCondition;
        static pthread_key_t sThreadKey;
        static pthread_once_t sThreadKeyOnce;
    };
};
#endif
