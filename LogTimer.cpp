// LogTimer.cpp
#include "LogTimer.h"

using namespace Forte;

LogTimer::LogTimer(int level, const FString& log_str) :
    mLevel(level), mLogStr(log_str)
{
    gettimeofday(&mStart, NULL);
}


LogTimer::~LogTimer()
{
    FString stmp;
    int m, s, us;

    // log end time
    gettimeofday(&mEnd, NULL);
    s = mEnd.tv_sec - mStart.tv_sec;
    us = mEnd.tv_usec - mStart.tv_usec;

    if (us < 0)
    {
        us += 1000000;
        s--;
    }

    if (s < 0) s = 0;
    m = s/60;
    s %= 60;

    stmp.Format("%02u:%02u.%03u", m, s, us/1000);
    hlog(mLevel, "%s", mLogStr.Replace("@TIME@", stmp).c_str());
}
