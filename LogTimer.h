#ifndef __forte_LogTimer_h
#define __forte_LogTimer_h

#include "LogManager.h"
#include <sys/time.h>
namespace Forte
{
    class LogTimer : public Object
    {
    public:
        LogTimer(int level, const FString& log_str);
        ~LogTimer();

    protected:
        int mLevel;
        FString mLogStr;
        struct timeval mStart, mEnd;
    };
};
#endif
