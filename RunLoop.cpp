#include "RunLoop.h"

Forte::RunLoop::RunLoop()
{

}

Forte::RunLoop::~RunLoop()
{

}

void Forte::RunLoop::AddTimer(shared_ptr<Timer> &timer)
{
    weak_ptr<Timer> weakTimer(timer);
    // add the weak reference
    // schedule it
}
