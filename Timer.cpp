// Timer.cpp
#include "Timer.h"
//#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

using namespace Forte;

//TODO: allow selection of clock type, allow specification of retry values

Timer::Timer()
    : mValidTimer(false)
{
}

Timer::~Timer()
{
    if (mValidTimer)
    {
        int ret_code;
        //hlog(HLOG_DEBUG4, "Timer Delete %u", mTimer);
        ret_code = timer_delete(mTimer);
        if (ret_code != 0)
        {
            // not sure if we can do anything about this but it'd be
            // nice to know
            hlog(HLOG_ERR, "Timer: Failed to delete timer. %u", ret_code);
        }
    }
}

// ctor/dtor
void Timer::Init(sigevent_t& se)
{
    if (mValidTimer)
    {
        throw ETimerInit("LOCK_TIMER_FAIL|||");
    }

    int err;

    // create timer
    int tries = 0;
    while ((err = timer_create(CLOCK_MONOTONIC, &se, &mTimer)) != 0)
    {
        if (err == -EAGAIN)
        {
            if (++tries < 100)
            {
                hlog(HLOG_DEBUG, "got EAGAIN from timer_create");
                usleep(100000); // TODO: should we subtract from the timeout?
                continue;
            }
            else
            {
                err = errno;
                throw ETimer("LOCK_TIMER_FAIL|||" +
                             FileSystem::get()->strerror(err));
            }
        }
        err = errno;

        throw ETimer(FileSystem::get()->strerror(err));
    }
    mValidTimer = true;
    //hlog(HLOG_DEBUG4, "Timer Create %u", mTimer);
}

timer_t Timer::TimerID()
{
    if (mValidTimer)
    {
        return mTimer;
    }
    else
    {
        throw ETimer("LOCK_TIMER_FAIL|||Attempt to use invalid timer");
    }
}
