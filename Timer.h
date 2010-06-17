#ifndef __forte__timer_h
#define __forte__timer_h

#include "Object.h"
#include "AutoMutex.h"
#include "Clock.h"
#include "Context.h"
#include <csignal>
#include <ctime>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

using namespace boost;

namespace Forte
{
    EXCEPTION_CLASS(ETimer);
    EXCEPTION_SUBCLASS2(ETimer, ETimerRunLoopInvalid, "Invalid RunLoop");
    EXCEPTION_SUBCLASS2(ETimer, ETimerTargetInvalid, "Invalid Target Object");
    EXCEPTION_SUBCLASS2(ETimer, ETimerIntervalInvalid, "Invalid Timer Interval");

    class RunLoop;

    class Timer : public Object
    {
    public:
        typedef boost::function<void(void)> Callback;

        /** 
         * Create a Timer object, which will call the callback method
         * on the target object at the given time.
         * 
         * @param runloop The RunLoop this timer should utilize.
         * @param target 
         * @param callback 
         * @param interval
         * @param repeat 
         * 
         * @return 
         */
        Timer(shared_ptr<RunLoop> runloop,
              shared_ptr<Object> target,
              Callback callback,
              Timespec interval,
              bool repeats = false);

        virtual ~Timer();

        Timespec GetInterval(void) { return mInterval; }

        bool Repeats(void) { return mRepeats; }
        
        void Fire(void) { mCallback(); }

    private:

        /** We take shared ownership of the run loop.  This prevents
         * our run loop from being deleted while the timer is still
         * pending, and prevents bugs where a pending timer never
         * fires.
         */
        shared_ptr<RunLoop> mRunLoop;
        shared_ptr<Object> mTarget;
        Callback mCallback;
        Timespec mInterval;
        bool mRepeats;
    };
}

#endif
