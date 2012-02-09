#ifndef __Forte_clock_h_
#define __Forte_clock_h_

#include "Exception.h"

namespace Forte
{
    EXCEPTION_CLASS(EClock);
    EXCEPTION_SUBCLASS(EClock, EClockInvalid);
    EXCEPTION_SUBCLASS(EClock, EClockPermission);

    EXCEPTION_CLASS(ETimespec);
    EXCEPTION_SUBCLASS2(ETimespec, ETimespecInvalid, "Invalid timespec");

    /**
     *The Timespec class is used to handle different representations of time. You use this class to
     *convert from one representation of time to another, for example converting to time in seconds or
     *milliseconds. You can create a Timespec object that handles @ref MonotonicClock time or
     *@ref RealtimeClock time. The class also provides methods for adding time values together or examining
     *time values to determine if they are equal to zero or positive.
     **/
    class Timespec : public Object
    {
    public:
        /**
         *Sets the Timespec object to zero.
         **/
        Timespec() {
            Clear();
        };
        Timespec(const Timespec &ts) {
            *this = ts;
        };

        /**
         *Makes a copy of a Timespec object for you.
         **/
        Timespec(const struct timespec &ts) {
            *this = ts;
        };

        Timespec(const long sec, const long nsec) {
            Set(sec,nsec);
        }

        virtual ~Timespec() {};

        Timespec & operator=(const Timespec &ts) {
            mTimespec.tv_sec = ts.mTimespec.tv_sec;
            mTimespec.tv_nsec = ts.mTimespec.tv_nsec;
            return *this;
        }
        /**
         *Assigns one Timespec object to another.
         **/
        Timespec operator=(const struct timespec &ts) {
            mTimespec.tv_sec = ts.tv_sec;
            mTimespec.tv_nsec = ts.tv_nsec;
            return *this;
        }

        /**
         *Test equality.
         **/
        bool operator == (const struct timespec &ts) const
        {
            if (mTimespec.tv_sec != ts.tv_sec) return false;
            if (mTimespec.tv_nsec != ts.tv_nsec) return false;
            return true;;
        }

        /**
         *Adds two Timespec objects together
         **/
        Timespec operator+(const Timespec &other) const;
        Timespec operator-(const Timespec &other) const;
        bool operator<(const Timespec& other) const;

        /**
         *Returns the current value of your Timespec object.
         **/
        operator const struct timespec () const { return mTimespec; }

        /**
         * Sets the values of sec and nsec
         */
        void Set(const long sec, const long nsec) {
            if (nsec < 0 || nsec > 999999999)
                throw ETimespecInvalid();
            mTimespec.tv_sec = sec;
            mTimespec.tv_nsec = nsec;
        }

        /**
         *Returns the time in milliseconds.
         **/
        long long AsMillisec(void) const {
            return (mTimespec.tv_sec * 1000) + (mTimespec.tv_nsec / 1000000);
        }

        /**
         *Takes milliseconds and converts them to a Timespec object which is returned to you.
         **/
        static Timespec FromMillisec(int ms) {
            Timespec ts;
            ts.mTimespec.tv_sec = ms / 1000;
            ts.mTimespec.tv_nsec = (ms % 1000) * 1000000;
            return ts;
        }

        /**
         *Takes seconds and converts them to a Timespec object which is returned to you.
         **/
        static Timespec FromSeconds(long long int sec) {
            Timespec ts;
            ts.mTimespec.tv_sec = sec;
            ts.mTimespec.tv_nsec = 0;
            return ts;
        }

        /**
         *Returns the time in seconds.
         **/
        long AsSeconds(void) const {
            return mTimespec.tv_sec;
        }

        long GetNanosecs(void) const {
            return mTimespec.tv_nsec;
        }

        /**
         *If your Timespec object is representing a time interval, this method returns whether or not
         *the interval is zero.
         **/
        bool IsZero(void) const {
            return (mTimespec.tv_sec == 0 && mTimespec.tv_nsec == 0);
        }

        /**
         *If your Timespec object is representing a time interval, this method returns whether or not
         *it is a positive time interval.
         **/
        bool IsPositive(void) const {
            return (mTimespec.tv_sec > 0 || (mTimespec.tv_sec == 0 && mTimespec.tv_nsec > 0));
        }

        /**
         *Sets the Timespec object to zero.
         **/
        void Clear(void) {
            mTimespec.tv_sec = 0;
            mTimespec.tv_nsec = 0;
        }

        /*
         * Gets a string representation
         */
        FString GetString(void) const {
            return FString(FStringFC(), "%lu.%09lu", mTimespec.tv_sec,
                           mTimespec.tv_nsec);
        }
    protected:
        struct timespec mTimespec;
    };

    /**
     * The Clock class is an interface for implementing subclasses
     * @ref MonotonicClock and @ref RealtimeClock.
     */
    class Clock : public Object
    {
    public:
        Clock() {};
        virtual ~Clock() {};

        /**
         * This is a pure virtual function that fills in the Timespec
         * object with the current time dependent upon the derived
         * class (MonotonicClock or RealtimeClock).
         **/
        virtual void GetTime(struct timespec &ts) const = 0;

        /**
         *Returns the time as a Timespec object.
         **/
        Timespec GetTime(void) const {
            struct timespec ts;
            GetTime(ts);
            return ts;
        };

        /**
         * If the clock can be converted to a realtime value,
         * this method will do so, or else throw EUnimplemented.
         */
        virtual Timespec ConvertToRealtime(const struct timespec &ts) const = 0;
        /**
         * If the clock can be converted to a realtime value, this
         * method will do so (preserving zero timespecs), or else
         * throw EUnimplemented.
         */
        virtual Timespec ConvertToRealtimePreserveZero(const struct timespec &ts) const = 0;
    };

    /**
     * The MonotonicClock class is used to create clocks where the
     * value of the time always increases even if the system time
     * changes. Time in a MonotonicClock object increases at a
     * consistent rate regardless of the system time changing. It is an
     * ideal class to use if you want to measure time intervals.
     */
    class MonotonicClock : public Clock
    {
    public:
        MonotonicClock() {};
        virtual ~MonotonicClock() {};

        /**
         *This is a virtual function that fills in the Timespec object with the current monotonic time.
         **/
        virtual void GetTime(struct timespec &ts) const;
        Timespec GetTime(void) const {
            return Clock::GetTime();
        };
        virtual Timespec ConvertToRealtime(const struct timespec &ts) const;
        virtual Timespec ConvertToRealtimePreserveZero(const struct timespec &ts) const;
    };

    /**
     *The DeadlineClock class is used to set a time in the future,
     *then be able to know if the the current time is past the deadline.
     **/
    class DeadlineClock : public MonotonicClock
    {
    public:
        DeadlineClock() {}
        //DeadlineClock(const struct timespec& deadlineTimespec) :
        //mDeadlineTimespec(deadlineTimespec) {}
        virtual ~DeadlineClock() {}

        virtual bool Expired() const;
        //TODO: virtual bool SetExpired(struct timespec &ts);
        virtual void ExpiresInSeconds(long long int sec);
        virtual void ExpiresInMillisec(int millisec);

        //TODO: virtual void GetTimeLeft(struct timespec &ts) const;
    protected:
        Timespec mDeadlineTimespec;
    };

    /**
     * The Process CPU clock measures the amount of cpu time for this process
     */
    class ProcessCPUClock : public Clock
    {
    public:
        ProcessCPUClock() {}
        virtual ~ProcessCPUClock() {}

        virtual void GetTime(struct timespec& ts) const;
        Timespec GetTime(void) const {
            return Clock::GetTime();
        }
        virtual Timespec ConvertToRealtime(const struct timespec &ts) const {
            throw EUnimplemented();
        }
        virtual Timespec ConvertToRealtimePreserveZero(const struct timespec &ts) const {
            throw EUnimplemented();
        }
    };

    /**
     * The RealtimeClock class is used to create clocks where the value
     * of time is the same as the system clock.  Time in a
     * RealtimeClock object is best for measuring accurate system times
     * and is susceptible to any system clock changes.
     */
    class RealtimeClock : public Clock
    {
    public:
        RealtimeClock() {};
        virtual ~RealtimeClock() {};

        /**
         * This is a virtual function that fills in the Timespec object
         * with the current time on the system clock.
         */
        virtual void GetTime(struct timespec &ts) const;
        Timespec GetTime(void) const {
            return Clock::GetTime();
        }
        virtual Timespec ConvertToRealtime(const struct timespec &ts) const {
            return ts;
        }
        virtual Timespec ConvertToRealtimePreserveZero(const struct timespec &ts) const {
            return ts;
        }

    };

    std::ostream& operator<<(std::ostream& os, const Timespec& obj);
};

namespace std
{

    template <>
    const Forte::Timespec& min (const Forte::Timespec& a, const Forte::Timespec& b);

    template <>
    const Forte::Timespec& max (const Forte::Timespec& a, const Forte::Timespec& b);

}

#endif
