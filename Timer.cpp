#include "Clock.h"
#include "FTrace.h"
#include "Timer.h"
#include "RunLoop.h"
#include <boost/shared_ptr.hpp>
using namespace Forte;
using namespace boost;

Timer::Timer(const Forte::FString& name,
             shared_ptr<RunLoop> runloop,
             shared_ptr<Object> target,
             Callback callback,
             Timespec interval,
             bool repeats) :
    mName(name),
    mRunLoop(runloop),
    mTarget(target),
    mCallback(callback),
    mInterval(interval),
    mRepeats(repeats)
{
    FTRACE;
    if (!runloop) throw ETimerRunLoopInvalid();
    if (!target) throw ETimerTargetInvalid();
}

Timer::~Timer()
{
    FTRACE;
}

