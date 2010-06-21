#include "RunLoop.h"
#include "LogManager.h"
#include "FTrace.h"

Forte::RunLoop::RunLoop()
{
    FTRACE;
    initialized();
}

Forte::RunLoop::~RunLoop()
{
    FTRACE;
}

void Forte::RunLoop::AddTimer(shared_ptr<Timer> &timer)
{
    FTRACE;
    if (!timer) throw ERunLoopTimerInvalid();

    AutoUnlockMutex lock(mLock);

    // schedule it
    MonotonicClock mc;
    mSchedule.insert(RunLoopScheduleItem(timer, mc.GetTime() + timer->GetInterval()));
    hlog(HLOG_DEBUG, "scheduled timer (now=%ld interval=%ld)", 
         mc.GetTime().AsSeconds(), timer->GetInterval().AsSeconds());
    // notify the run loop thread
    Notify();
}

void * Forte::RunLoop::run(void)
{
    mThreadName.Format("runloop-%u", GetThreadID());
    hlog(HLOG_DEBUG, "runloop starting");
    MonotonicClock mc; 
    std::multiset<RunLoopScheduleItem>::iterator i;
    AutoUnlockMutex lock(mLock);
    bool warned = false;
    while (!IsShuttingDown())
    {
        Timespec now = mc.GetTime();
        i = mSchedule.begin();
        if (i == mSchedule.end())
        {
            interruptibleSleep(Timespec::FromSeconds(60));
            continue;
        }
        // we have a valid schedule item
        Timespec fireTime = i->GetAbsolute();
        Timespec waitInterval = fireTime - now;
        if (waitInterval.IsPositive())
        {
            // sleep in an unlocked scope
            AutoLockMutex unlock(mLock);
            interruptibleSleep(waitInterval);
        }
        else
        {
            Timespec latency = now - fireTime;
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
                hlog(HLOG_DEBUG, "timer destroyed");
                continue;
            }

            // fire the timer in an unlocked scope
            {
                AutoLockMutex unlock(mLock);
                timer->Fire();
            }

            if (timer->Repeats())
            {
                Timespec next;
                // If our latency is so high that the next firing time
                // would be in the past, just compute the next time
                // from now.
                if (timer->GetInterval() < latency)
                    next = now + timer->GetInterval();
                else
                    next = fireTime + timer->GetInterval();
                mSchedule.insert(RunLoopScheduleItem(timer, next));
            }
        }
        if (warned)
        {
            hlog(HLOG_WARN, "run loop has caught up");
            warned = false;
        }
    }
    return NULL;
}
