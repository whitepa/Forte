#ifndef __forte__timer_h
#define __forte__timer_h

#include "Object.h"
#include "Types.h"
#include "AutoMutex.h"
#include "FileSystem.h"
#include <csignal>
#include <ctime>

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, ETimer);
    EXCEPTION_SUBCLASS(ETimer, ETimerInit);

    class Timer : public Object
    {
    public:
        Timer();
        virtual ~Timer();
    
        void Init(sigevent_t& se);
        timer_t TimerID();
    
    protected:
        //TODO: pull this from the application context
        FileSystem mFileSystem;
        timer_t mTimer;
        bool mValidTimer;
    };
    
}

#endif
