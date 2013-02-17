#include "Dispatcher.h"
#include "FTrace.h"

using namespace Forte;

Forte::Dispatcher::Dispatcher(boost::shared_ptr<RequestHandler> reqHandler, 
                              int maxQueueDepth, const char *name) :
    mDispatcherName(name),
    mPaused(false),
    mShutdown(false),
    mRequestHandler(reqHandler),
    mNotify(mNotifyLock),
    mEventQueue(maxQueueDepth, &mNotify)
{
    FTRACE;

    if (!mRequestHandler)
        throw EDispatcherReqHandlerInvalid();
}
Forte::Dispatcher::~Dispatcher()
{
    FTRACE;
}

DispatcherThread::DispatcherThread(Dispatcher &dispatcher) :
    mDispatcher(dispatcher)
{
    FTRACE;
}

DispatcherThread::~DispatcherThread()
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
