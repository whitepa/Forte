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
        void AddTimer(shared_ptr<Timer> &timer);

    private:
        virtual void * run(void);

        Mutex mLock;

        class RunLoopScheduleItem
        {
        public:
            RunLoopScheduleItem(shared_ptr<Timer> &timer,
                                Timespec absolute) :
                mTimer(timer), mAbsolute(absolute) {}
            bool operator < (const RunLoopScheduleItem &other) const { 
                return mAbsolute < other.mAbsolute;
            }
            shared_ptr<Timer>GetTimer(void) const { return mTimer.lock(); }
            const Timespec & GetAbsolute(void) const { return mAbsolute; }
            weak_ptr<Timer> mTimer;
            Timespec mAbsolute;
        };
        std::multiset<RunLoopScheduleItem> mSchedule;
    };
};

#endif

