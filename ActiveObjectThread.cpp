#include "ActiveObjectThread.h"
#include "Clock.h"

#define MAX_QUEUED_CALLS 128

using namespace boost;
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
    if (IsShuttingDown())
        throw_exception(EActiveObjectThreadShuttingDown());
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

void ActiveObjectThread::DropQueue(void)
{
    boost::shared_ptr<Event> event;
    while((event = mQueue.Get()))
    {
        boost::shared_ptr<Forte::AsyncInvocation> invocation =
            dynamic_pointer_cast<AsyncInvocation>(event);
        event.reset();
        if (invocation)
        {
            invocation->Drop();
        }
    }
}
void ActiveObjectThread::CancelRunning(void)
{
    AutoUnlockMutex lock(mLock);

    if (mCurrentAsyncInvocation)
        mCurrentAsyncInvocation->Cancel();
}

void ActiveObjectThread::WaitUntilQueueEmpty(void)
{
    mQueue.WaitUntilEmpty();
}

void * ActiveObjectThread::run(void)
{
    FTRACE;
    boost::shared_ptr<Event> event;
    // Always attempt to drain the queue, even if shutting down.  If
    // the caller desires immediate shutdown, ActiveObject::Shutdown()
    // with the appropriate parameters will have been called, and the
    // queue will be immediately cleared.
    while (!IsShuttingDown() || mQueue.Depth() > 0)
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
        interruptibleSleep(Timespec::FromSeconds(1), false);
    }
    return NULL;
}
