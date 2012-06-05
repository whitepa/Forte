#include "Clock.h"
#include <time.h>
#include <errno.h>
#include "Util.h"
#include "LogManager.h"

using namespace Forte;

void Forte::MonotonicClock::GetTime(struct timespec &ts) const
{
    int err = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (err == -EINVAL)
        throw EClockInvalid();
    else if (err == -EFAULT)
        throw EClock("Bad address");
}
Timespec Forte::MonotonicClock::ConvertToRealtime(const struct timespec &ts) const
{
    return ts + (RealtimeClock().GetTime() - GetTime());
}
Timespec Forte::MonotonicClock::ConvertToRealtimePreserveZero(const struct timespec &ts) const
{
    Timespec tSpec(ts);
    if (tSpec.IsZero())
        return tSpec;
    return tSpec + (RealtimeClock().GetTime() - GetTime());
}

void Forte::RealtimeClock::GetTime(struct timespec &ts) const
{
    int err = clock_gettime(CLOCK_REALTIME, &ts);
    if (err == -EINVAL)
        throw EClockInvalid();
    else if (err == -EFAULT)
        throw EClock("Bad address");
}

void Forte::ProcessCPUClock::GetTime(struct timespec &ts) const
{
    int err = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
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

bool Timespec::operator<(const Timespec& other) const
{
    return (mTimespec < other.mTimespec);
}

bool Timespec::operator<=(const Timespec& other) const
{
    return (mTimespec <= other.mTimespec);
}

bool DeadlineClock::Expired() const
{
    //hlogstream(HLOG_DEBUG, "deadline:" << mDeadlineTimespec
    //<< " time: " << GetTime());
    return mDeadlineTimespec < GetTime(); // <= ?
}

void DeadlineClock::ExpiresInSeconds(long long int sec)
{
    mDeadlineTimespec = GetTime() + Timespec::FromSeconds(sec);
}

void DeadlineClock::ExpiresInMillisec(int millisec)
{
    mDeadlineTimespec = GetTime() + Timespec::FromMillisec(millisec);
}


namespace Forte
{

std::ostream& operator<<(std::ostream& os, const Timespec& obj)
{
    os << obj.GetString();
    return os;
}

}

namespace std
{

    template <>
    const Forte::Timespec& min (const Forte::Timespec& a, const Forte::Timespec& b)
    {
        return (a < b ? a : b);
    }

    template <>
    const Forte::Timespec& max (const Forte::Timespec& a, const Forte::Timespec& b)
    {
        return (a < b ? b : a);
    }
}
