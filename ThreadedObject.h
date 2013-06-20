#ifndef __Forte_ThreadedObject_h__
#define __Forte_ThreadedObject_h__

#include "Exception.h"
#include "FTrace.h"

/**
 * This interface provides a common way for Objects that own
 * Forte::Threads (or any threads) to indicate they have two stage
 * construction/destruction. it's purpose is to go around the problem
 * of passing an object::function to a thread before the object is
 * fully constructed, and shutting down the thread from the object's
 * destructor, which is technically too late.
 */

namespace Forte
{
    EXCEPTION_SUBCLASS2(
        Exception,
        EObjectNotRunning,
        "Method called requires ThreadedObject class to be running");

    class ThreadedObject : public Forte::Object
    {
    public:
        ThreadedObject()
            : mStartCalled(0),
              mShutdownCalled(0) {
            FTRACE;
        }

        virtual ~ThreadedObject() {
            FTRACE;
            if (mShutdownCalled != mStartCalled)
            {
                hlogstream(HLOG_CRIT, "Software error: "
                           "shutdown not called the same number of times as start"
                           << " start: " << mStartCalled
                           << " shutdown: " << mShutdownCalled);
                abort();
            }

            if (mStartCalled == 0)
            {
                hlog(HLOG_WARN, "ThreadedObject created but was never started");
            }
        }

        /**
         * gentlemen, start your threads
         */
        virtual void Start() = 0;

        /**
         * hopefully there is no winner, because that would mean there
         * was a race. sub classes need to call recordShutdownCall()
         * to use this class properly
         */
        virtual void Shutdown() = 0;

    protected:
        void recordShutdownCall() {
            FTRACE;
            ++mShutdownCalled;

            if (mShutdownCalled != mStartCalled)
            {
                hlogstream(HLOG_CRIT, "Software error: "
                           "shutdown called more than start"
                           << " start: " << mStartCalled
                           << " shutdown: " << mShutdownCalled);
                abort();
            }
        }

        void recordStartCall() {
            FTRACE;
            ++mStartCalled;

            if (mStartCalled != (mShutdownCalled+1))
            {
                hlogstream(HLOG_CRIT, "Software error: "
                           "start called while running"
                           << " start: " << mStartCalled
                           << " shutdown: " << mShutdownCalled);
                abort();
            }
        }

        bool isRunning() {
            return (mStartCalled > mShutdownCalled);
        }

    private:
        int mStartCalled;
        int mShutdownCalled;
    };
}

#endif /* __Forte_ThreadedObject_h__ */

