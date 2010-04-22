#ifndef __Forte_clock_h_
#define __Forte_clock_h_

#include "Exception.h"

namespace Forte
{
    EXCEPTION_CLASS(EClock);
    EXCEPTION_SUBCLASS(EClock, EClockInvalid);
    EXCEPTION_SUBCLASS(EClock, EClockPermission);
    
    class Clock : public Object
    {
    public:
        Clock() {};
        virtual ~Clock() {};
        
        virtual void GetTime(struct timespec &ts) const = 0;
    };

    class MonotonicClock : public Clock
    {
    public:
        MonotonicClock() {};
        virtual ~MonotonicClock() {};

        virtual void GetTime(struct timespec &ts) const;
    };

};
#endif
