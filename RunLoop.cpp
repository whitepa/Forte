#include "RunLoop.h"

Forte::RunLoop::RunLoop()
{
    
}

Forte::RunLoop::~RunLoop()
{

}

void Forte::RunLoop::AddTimer(shared_ptr<Timer> &timer)
{
    if (!timer) throw ERunLoopTimerInvalid();

    AutoUnlockMutex lock(mLock);

    // schedule it
    MonotonicClock mc;
    mSchedule.insert(RunLoopScheduleItem(timer, mc.GetTime() + timer->GetInterval()));
}

void * Forte::RunLoop::run(void)
{
    // while 1
    //   interruptible sleep until the next timer
    //   check schedule, call all expired timers
}
