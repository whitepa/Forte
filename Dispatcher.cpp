#include "Forte.h"

Forte::Dispatcher::Dispatcher() :
    mPaused(false),
    mShutdown(false)
{}
Forte::Dispatcher::~Dispatcher()
{
    // XXX
}

void Forte::Dispatcher::registerThread(DispatcherThread *thr)
{
    AutoUnlockMutex lock(mThreadsLock); 
    mThreads.push_back(thr);
}
void Forte::Dispatcher::unregisterThread(DispatcherThread *thr)
{
    hlog(HLOG_DEBUG, "unregisterThread()");
    AutoUnlockMutex lock(mThreadsLock);
    std::vector<DispatcherThread*>::iterator i;
    for (i = mThreads.begin(); i!=mThreads.end(); ++i)
    {
        if (*i == thr)
        {
            mThreads.erase(i);
            break;
        }
    }
}
//DispatcherThread::DispatcherThread() :
//    Thread(true) // use self deleting threads
//{}
//    // detach ourselves, since no one will be joining us
//    pthread_detach(mThread);
//}
DispatcherThread::DispatcherThread() :
    mEventPtr(NULL)
{}
DispatcherThread::~DispatcherThread()
{
    // XXX
}
