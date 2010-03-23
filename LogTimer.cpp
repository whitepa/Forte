// LogTimer.cpp
#include "Forte.h"


LogTimer::LogTimer(int level, const FString& log_str) :
    m_level(level), m_log_str(log_str)
{
    gettimeofday(&m_start, NULL);
}


LogTimer::~LogTimer()
{
    FString stmp;
    int m, s, us;

    // log end time
    gettimeofday(&m_end, NULL);
    s = m_end.tv_sec - m_start.tv_sec;
    us = m_end.tv_usec - m_start.tv_usec;

    if (us < 0)
    {
        us += 1000000;
        s--;
    }

    if (s < 0) s = 0;
    m = s/60;
    s %= 60;

    stmp.Format("%02u:%02u.%03u", m, s, us/1000);
    hlog(m_level, "%s", m_log_str.Replace("@TIME@", stmp).c_str());
}
