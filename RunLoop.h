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
            bool operator < (const RunLoopScheduleItem &other) const { 
                return absolute < other.absolute;
            }

            weak_ptr<Timer> timer;
            Timespec absolute;
        };

        std::set<RunLoopScheduleItem> mUpcomingSet;



    };

};

#endif

