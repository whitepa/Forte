#include "Clock.h"
#include "Timer.h"
#include "RunLoop.h"
#include <boost/shared_ptr.hpp>
using namespace Forte;
using namespace boost;

Timer::Timer(shared_ptr<RunLoop> runloop,
             shared_ptr<Object> target,
             Callback callback,
             Timespec when,
             bool repeat)
{
    
}

Timer::~Timer()
{
}

