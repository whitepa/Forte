#include "ActiveObjectThread.h"
#include "Clock.h"

#define MAX_QUEUED_CALLS 128

using namespace Forte;

ActiveObjectThread::ActiveObjectThread() :
    mQueue(MAX_QUEUED_CALLS)
{
    setThreadName("active");
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

bool ActiveObjectThread::IsCancelled(void)
{
    AutoUnlockMutex lock(mLock);
    if (!mCurrentAsyncInvocation)
        throw EActiveObjectNoCurrentInvocation();
    else
        return mCurrentAsyncInvocation->IsCancelled();
}

void ActiveObjectThread::SetName(const FString &name)
{
    setThreadName(FString(FStringFC(), "active-%s", name.c_str()));
}

void * ActiveObjectThread::run(void)
{
    shared_ptr<Event> event;
    while (!IsShuttingDown())
    {
        while((event = mQueue.Get()))
        {
            AutoUnlockMutex lock(mLock);
            mCurrentAsyncInvocation =
                dynamic_pointer_cast<AsyncInvocation>(event);
            event.reset();
            if (!mCurrentAsyncInvocation)
                throw EActiveObjectThreadReceivedInvalidEvent();
            {
                AutoLockMutex unlock(mLock);
                mCurrentAsyncInvocation->Execute();
            }
            mCurrentAsyncInvocation.reset();
        }
        // @TODO this is racy... we may sleep when there is an event waiting.
        interruptibleSleep(Timespec::FromSeconds(1));
    }
    return NULL;
}
