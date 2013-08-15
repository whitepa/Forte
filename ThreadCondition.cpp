#include "ThreadCondition.h"

// Standard C++
#include <iostream>


using namespace Forte;


static void ReportError(const char* prefix, int errnum)
{
    char buf[256];
    strerror_r(errnum, buf, sizeof(buf));
    buf[sizeof(buf) - 1] = 0;

    std::cerr << prefix << buf << std::endl;
}

class ThreadConditionAttr
{
private:
    // non-copyable
    ThreadConditionAttr           (const ThreadConditionAttr&);
    ThreadConditionAttr& operator=(const ThreadConditionAttr&);

public:
    ThreadConditionAttr()
    {
        if (pthread_condattr_init(&mCondAttr) != 0)
        {
            throw EThreadCondition();
        }
    }

    ~ThreadConditionAttr()
    {
        if (int err = pthread_condattr_destroy(&mCondAttr))
        {
            ReportError("pthread_condattr_destroy failed with ", err);
        }
    }

    const pthread_condattr_t& Get() const  { return mCondAttr; }

protected:
    pthread_condattr_t mCondAttr;
};

class MonotonicClockThreadConditionAttr : public ThreadConditionAttr
{
public:
    MonotonicClockThreadConditionAttr()
    {
        if (pthread_condattr_setclock(&mCondAttr, CLOCK_MONOTONIC) != 0)
        {
            throw EThreadCondition();
        }
    }
};

ThreadCondition::ThreadCondition(Mutex& lock) : mLock(lock)
{
    MonotonicClockThreadConditionAttr attr;

    if (pthread_cond_init(&mCond, &attr.Get()) != 0)
    {
        throw EThreadCondition();
    }
}

ThreadCondition::~ThreadCondition()
{
    if (int err = pthread_cond_destroy(&mCond))
    {
        ReportError("pthread_cond_destroy failed with ", err);
    }
}

int ThreadCondition::TimedWait(int seconds)
{
    struct timespec ts;
    MonotonicClock().GetTime(ts);
    ts.tv_sec += seconds;
    return TimedWait(ts);
}
