#ifndef __forte_LogTimer_h
#define __forte_LogTimer_h

#include "LogManager.h"
#include <sys/time.h>

class CLogTimer
{
public:
    CLogTimer(int level, const FString& log_str);
    ~CLogTimer();

protected:
    int m_level;
    FString m_log_str;
    struct timeval m_start, m_end;
};

#endif
