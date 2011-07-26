#ifndef __Forte_RunLoop_h__
#define __Forte_RunLoop_h__

#include "Clock.h"
#include "Context.h"
#include "Thread.h"
#include "Timer.h"
#include "Util.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

using namespace boost;

namespace Forte
{
    EXCEPTION_CLASS(ERunLoop);
    EXCEPTION_SUBCLASS2(ERunLoop, ERunLoopTimerInvalid, "Invalid Timer");
    
    class RunLoop : public Thread
    {
    public:

        RunLoop();
        virtual ~RunLoop();

        /** 
         * AddTimer() allows the addition of Timer objects to the RunLoop.
         * 
         * @param timer
         */
        void AddTimer(const shared_ptr<Timer> &timer);

        /**
         * IsEmpty() checks to see if there is anything scheduled to run
         */
        bool IsEmpty() const;

    private:
        virtual void * run(void);

        mutable Mutex mLock;

        class RunLoopScheduleItem
        {
        public:
            RunLoopScheduleItem(const shared_ptr<Timer> &timer,
                                const Timespec &absolute) :
                mTimer(timer), mAbsolute(absolute) {
                mScheduledTime = Forte::MonotonicClock().GetTime();
            }
            bool operator < (const RunLoopScheduleItem &other) const { 
                return mAbsolute < other.mAbsolute;
            }
            shared_ptr<Timer>GetTimer(void) const { return mTimer.lock(); }
            const Timespec & GetAbsolute(void) const { return mAbsolute; }
            const Timespec & GetScheduledTime(void) const { return mScheduledTime; }
            weak_ptr<Timer> mTimer;
            Timespec mAbsolute;
            Timespec mScheduledTime;
        };
        std::multiset<RunLoopScheduleItem> mSchedule;
    };
};

#endif

