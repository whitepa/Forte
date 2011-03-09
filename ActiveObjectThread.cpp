#include "ActiveObjectThread.h"
#include "Clock.h"

#define MAX_QUEUED_CALLS 128

using namespace Forte;

ActiveObjectThread::ActiveObjectThread() :
    mQueue(MAX_QUEUED_CALLS)
{
    initialized();
}
ActiveObjectThread::~ActiveObjectThread()
{
    deleting();
}

void ActiveObjectThread::Enqueue(const boost::shared_ptr<AsyncInvocation> &ai)
{
    mQueue.Add(ai);
    Notify();
}

void * ActiveObjectThread::run(void)
{
    shared_ptr<Event> event;
    while (!IsShuttingDown())
    {
        while((event = mQueue.Get()))
        {
            shared_ptr<AsyncInvocation> ai =
                dynamic_pointer_cast<AsyncInvocation>(event);
            event.reset();
            if (!ai)
                throw EActiveObjectThreadReceivedInvalidEvent();
            ai->Execute();
        }
        // @TODO this is racy... we may sleep when there is an event waiting.
        interruptibleSleep(Timespec::FromSeconds(1));
    }
    return NULL;
}
