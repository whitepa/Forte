#include "Clock.h"
#include <time.h>
#include <errno.h>
#include "Util.h"

using namespace Forte;

void Forte::MonotonicClock::GetTime(struct timespec &ts) const
{
    int err = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (err == -EINVAL)
        throw EClockInvalid();
    else if (err == -EFAULT)
        throw EClock("Bad address");
}

void Forte::RealtimeClock::GetTime(struct timespec &ts) const
{
    int err = clock_gettime(CLOCK_REALTIME, &ts);
    if (err == -EINVAL)
        throw EClockInvalid();
    else if (err == -EFAULT)
        throw EClock("Bad address");
}

Timespec Timespec::operator + (const Timespec &other) const
{
    return Timespec(mTimespec + other.mTimespec);
}

Timespec Timespec::operator - (const Timespec &other) const
{
    return Timespec(mTimespec - other.mTimespec);
}
