#include "Forte.h"

Forte::Dispatcher::Dispatcher(boost::shared_ptr<RequestHandler> reqHandler, 
                              int maxQueueDepth, const char *name) :
    mDispatcherName(name),
    mPaused(false),
    mShutdown(false),
    mRequestHandler(reqHandler),
    mNotify(mNotifyLock),
    mEventQueue(maxQueueDepth, &mNotify)
{
    if (!mRequestHandler)
        throw EDispatcherReqHandlerInvalid();
}
Forte::Dispatcher::~Dispatcher()
{}
DispatcherThread::DispatcherThread(Dispatcher &dispatcher) :
    mDispatcher(dispatcher)
{}
DispatcherThread::~DispatcherThread()
{}
