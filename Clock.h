#ifndef __Forte_clock_h_
#define __Forte_clock_h_

#include "Exception.h"

namespace Forte
{
    EXCEPTION_CLASS(EClock);
    EXCEPTION_SUBCLASS(EClock, EClockInvalid);
    EXCEPTION_SUBCLASS(EClock, EClockPermission);
    
    class Timespec : public Object
    {
    public:
        Timespec() { 
            mTimespec.tv_sec = 0; 
            mTimespec.tv_nsec = 0;
        };
        Timespec(const struct timespec &ts) {
            mTimespec.tv_sec = ts.tv_sec;
            mTimespec.tv_nsec = ts.tv_nsec;
        };
        virtual ~Timespec() {};
        operator const struct timespec () const { return mTimespec; }
        unsigned long long AsMillisec(void) const {
            return (mTimespec.tv_sec * 1000) + (mTimespec.tv_nsec / 1000);
        }
        static Timespec FromMillisec(unsigned int ms) {
            Timespec ts;
            ts.mTimespec.tv_sec = ms / 1000;
            ts.mTimespec.tv_nsec = (ms % 1000) * 1000000;
            return ts;
        }
        bool IsZero(void) const {
            return (mTimespec.tv_sec == 0 && mTimespec.tv_nsec == 0);
        }
    protected:
        struct timespec mTimespec;
    };

    class Clock : public Object
    {
    public:
        Clock() {};
        virtual ~Clock() {};
        
        virtual void GetTime(struct timespec &ts) const = 0;
        struct timespec GetTime(void) const { 
            struct timespec ts;
            GetTime(ts);
            return ts;
        };

    };

    class MonotonicClock : public Clock
    {
    public:
        MonotonicClock() {};
        virtual ~MonotonicClock() {};

        virtual void GetTime(struct timespec &ts) const;
        struct timespec GetTime(void) const { 
            return Clock::GetTime();
        };
    };
    class RealtimeClock : public Clock
    {
    public:
        RealtimeClock() {};
        virtual ~RealtimeClock() {};

        virtual void GetTime(struct timespec &ts) const;
        struct timespec GetTime(void) const { 
            return Clock::GetTime();
        };
    };

};
#endif
