// Posixtimer.cpp
#include "PosixTimer.h"
//#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

using namespace Forte;

//TODO: allow selection of clock type, allow specification of retry values

PosixTimer::PosixTimer()
    : mValidPosixTimer(false)
{
}

PosixTimer::~PosixTimer()
{
    if (mValidPosixTimer)
    {
        int ret_code;
        //hlog(HLOG_DEBUG4, "Timer Delete %u", mPosixTimer);
        ret_code = timer_delete(mPosixTimer);
        if (ret_code != 0)
        {
            // not sure if we can do anything about this but it'd be
            // nice to know
            hlog(HLOG_ERR, "PosixTimer: Failed to delete timer. %u", ret_code);
        }
    }
}

// ctor/dtor
void PosixTimer::Init(sigevent_t& se)
{
    if (mValidPosixTimer)
    {
        throw EPosixTimerInit("LOCK_TIMER_FAIL|||");
    }

    int err;

    // create posixtimer
    int tries = 0;
    while ((err = timer_create(CLOCK_MONOTONIC, &se, &mPosixTimer)) != 0)
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
                throw EPosixTimer("LOCK_TIMER_FAIL|||" +
                             mFileSystem.StrError(err));
            }
        }
        err = errno;

        throw EPosixTimer(mFileSystem.StrError(err));
    }
    mValidPosixTimer = true;
    //hlog(HLOG_DEBUG4, "Timer Create %u", mPosixTimer);
}

timer_t PosixTimer::PosixTimerID()
{
    if (mValidPosixTimer)
    {
        return mPosixTimer;
    }
    else
    {
        throw EPosixTimer("LOCK_TIMER_FAIL|||Attempt to use invalid timer");
    }
}
