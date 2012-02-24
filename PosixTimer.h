#ifndef __forte__posixtimer_h
#define __forte__posixtimer_h

#include "Object.h"
#include "Types.h"
#include "AutoMutex.h"
#include "FileSystemImpl.h"
#include <csignal>
#include <ctime>

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, EPosixTimer);
    EXCEPTION_SUBCLASS(EPosixTimer, EPosixTimerInit);

    class PosixTimer : public Object
    {
    public:
        PosixTimer();
        virtual ~PosixTimer();

        void Init(sigevent_t& se);
        timer_t PosixTimerID();

    protected:
        //TODO: pull this from the application context
        FileSystemImpl mFileSystem;
        timer_t mPosixTimer;
        bool mValidPosixTimer;
    };

}

#endif
