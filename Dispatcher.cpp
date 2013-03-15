#include "Dispatcher.h"
#include "FTrace.h"

using namespace Forte;

////////////////////////////////////////////////////////////////////////////////
// Dispatcher
Forte::Dispatcher::Dispatcher(
    boost::shared_ptr<RequestHandler> reqHandler,
    int maxQueueDepth,
    const char *name)
    : mDispatcherName(name),
      mPaused(false),
      mShutdown(false),
      mRequestHandler(reqHandler),
      mNotify(mNotifyLock),
      mEventQueue(maxQueueDepth, &mNotifyLock, &mNotify)
{
    FTRACE;

    if (!mRequestHandler)
        throw EDispatcherReqHandlerInvalid();
}

Forte::Dispatcher::~Dispatcher()
{
    FTRACE;
}

void Forte::Dispatcher::Shutdown(void)
{
    FTRACE;
    // stop accepting new events
    mEventQueue.Shutdown();
    // set the shutdown flag
    AutoUnlockMutex lock(mNotifyLock);
    mShutdown = true;
}

// End Dispatcher
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// DispatcherThread
DispatcherThread::DispatcherThread(Dispatcher &dispatcher)
    : mDispatcher(dispatcher)
{
    FTRACE;
}

DispatcherThread::~DispatcherThread()
{
    FTRACE;
}

// End DispatcherThread
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// DispatcherWorkerThread
DispatcherWorkerThread::DispatcherWorkerThread(
    Dispatcher &dispatcher,
    const boost::shared_ptr<Event>& e)
    : DispatcherThread(dispatcher),
      mEventPtr(e)
{
    FTRACE;
}

DispatcherWorkerThread::~DispatcherWorkerThread()
{
    FTRACE;
}
// End DispatcherWorkerThread
////////////////////////////////////////////////////////////////////////////////
