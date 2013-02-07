#include "RunLoop.h"
#include "LogManager.h"
#include "FTrace.h"

using namespace boost;

Forte::RunLoop::RunLoop(const Forte::FString &name) :
        mName(name)
{
    FTRACE;
    initialized();
}

Forte::RunLoop::~RunLoop()
{
    FTRACE;
    deleting();
}

void Forte::RunLoop::AddTimer(const shared_ptr<Timer> &timer)
{
    FTRACE;
    if (!timer) throw ERunLoopTimerInvalid();

    AutoUnlockMutex lock(mLock);

    // schedule it
    MonotonicClock mc;
    mSchedule.insert(RunLoopScheduleItem(timer, mc.GetTime() + timer->GetInterval()));
    hlog(HLOG_DEBUG, "scheduled timer (now=%ld interval=%ld.%llu)",
         mc.GetTime().AsSeconds(), timer->GetInterval().AsSeconds(),
         timer->GetInterval().AsMillisec() % 1000);
    // notify the run loop thread
    Notify();
}

bool Forte::RunLoop::IsEmpty() const
{
    AutoUnlockMutex lock(mLock);
    return (mSchedule.begin() == mSchedule.end());
}

void * Forte::RunLoop::run(void)
{
    mThreadName.Format("runloop-%s-%u", mName.c_str(), GetThreadID());
    hlog(HLOG_DEBUG, "runloop starting");
    MonotonicClock mc;
    std::multiset<RunLoopScheduleItem>::iterator i;
    AutoUnlockMutex lock(mLock);
    bool warned = false;
    bool loopLatencyResolved = false;
    while (!IsShuttingDown())
    {
        Timespec now = mc.GetTime();
        i = mSchedule.begin();
        if (i == mSchedule.end())
        {
            // sleep in an unlocked scope
            AutoLockMutex unlock(mLock);
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
            Timespec fireLatency = now - fireTime;
            Timespec schedLatency = now - i->GetScheduledTime();
            Timespec latency;
            if (fireLatency < schedLatency)
                latency = fireLatency;
            else
                latency = schedLatency;

            if (latency.AsMillisec() > 100)
            {
                if(! warned)
                {
                    if (hlog_ratelimit(86400))
                    {
                        hlogstream(HLOG_WARN, "run loop has high latency of (" <<
                                   latency.AsMillisec() << "ms) prior to timer '" <<
                                   i->GetTimerName() << "'");
                    }
                    warned = true;
                }
            }
            else
            {
                if(warned)
                {
                    warned = false;
                    loopLatencyResolved = true;
                }
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
                try
                {
                    timer->Fire();
                }
                catch (std::exception &e)
                {
                    hlog(HLOG_WARN, "exception in timer callback: %s",
                         e.what());
                }
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

        if (loopLatencyResolved)
        {
            if (hlog_ratelimit(86400))
            {
                hlogstream(HLOG_WARN, "run loop has caught up for '" <<
                           mName << "'");
            }
            loopLatencyResolved = false;
        }
    }

    return NULL;
}
