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
         * AddTimer() is to be called by Timer objects when adding
         * themselves to the run loop.
         * 
         * @param timer
         */
        void AddTimer(shared_ptr<Timer> &timer);

    private:
        virtual void * run(void);

        Mutex mLock;
        Timespec mNext;

        class RunLoopScheduleItem
        {
        public:
            RunLoopScheduleItem(shared_ptr<Timer> &timer,
                                Timespec absolute) :
                mTimer(timer), mAbsolute(absolute) {}
            bool operator < (const RunLoopScheduleItem &other) const { 
                return mAbsolute < other.mAbsolute;
            }

            weak_ptr<Timer> mTimer;
            Timespec mAbsolute;
        };
        std::set<RunLoopScheduleItem> mSchedule;
    };
};

#endif

