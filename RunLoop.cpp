#include "RunLoop.h"
#include "LogManager.h"

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

    // notify the run loop thread
    Notify();
}

void * Forte::RunLoop::run(void)
{
    mThreadName.Format("runloop-%u", GetThreadID());
    MonotonicClock mc; 
    std::multiset<RunLoopScheduleItem>::iterator i;
    AutoUnlockMutex lock(mLock);
    i = mSchedule.begin();
    while (!IsShuttingDown())
    {
        Timespec now = mc.GetTime();
        if (i != mSchedule.end())
            mNext = i->GetAbsolute();
        else // run loop empty, wait a minute
            mNext = now + Timespec::FromSeconds(60);
        Timespec interval = mNext - now;
        if (interval.IsPositive())
        {
            // sleep in an unlocked scope
            AutoLockMutex unlock(mLock);
            interruptibleSleep(interval);
        }
        i = mSchedule.begin();
        if (i == mSchedule.end())
            continue; // run loop is empty

        bool warned = false;
        while (i->GetAbsolute() < mc.GetTime())
        {
            Timespec latency = mc.GetTime() - i->GetAbsolute();
            if (!warned && latency.AsMillisec() > 100)
            {
                hlog(HLOG_WARN, "run loop has high latency (%lld ms)",
                     latency.AsMillisec());
                warned = true;
            }
            shared_ptr<Timer> timer(i->GetTimer());
            mSchedule.erase(i);
            if (!timer)
            {
                // Timer has been destroyed, continue on
                continue;
            }

            // fire the timer in an unlocked scope
            {
                AutoLockMutex unlock(mLock);
                hlog(HLOG_DEBUG, "firing timer");
                timer->Fire();
            }

            if (timer->Repeats())
            {
                Timespec next = i->GetAbsolute() + timer->GetInterval();
                mSchedule.insert(RunLoopScheduleItem(timer, next));
            }
            i = mSchedule.begin();
        }
        if (warned)
            hlog(HLOG_WARN, "run loop has caught up");
    }
    return NULL;
}
