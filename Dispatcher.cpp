#include "Forte.h"

Forte::Dispatcher::Dispatcher(RequestHandler &reqHandler, int maxQueueDepth, const char *name) :
    mDispatcherName(name),
    mPaused(false),
    mShutdown(false),
    mRequestHandler(reqHandler),
    mNotify(mNotifyLock),
    mEventQueue(maxQueueDepth, &mNotify)
{}
Forte::Dispatcher::~Dispatcher()
{}
DispatcherThread::DispatcherThread(Dispatcher &dispatcher) :
    mDispatcher(dispatcher)
{}
DispatcherThread::~DispatcherThread()
{}
