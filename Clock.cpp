#include "Clock.h"
#include <time.h>
#include <errno.h>

void Forte::MonotonicClock::GetTime(struct timespec &ts) const
{
    int err = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (err == -EINVAL)
        throw EClockInvalid();
    else if (err == -EFAULT)
        throw EClock("Bad address");
}
