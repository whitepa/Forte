#include "EventQueue.h"
#include "AutoMutex.h"
#include "Exception.h"
#define EVQ_MAX_DEPTH 65536

CEventQueue::CEventQueue() :
    mBlockingMode(true),
    mShutdown(false),
    mMaxDepth(EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex)
{
    mNotify = NULL;
}

CEventQueue::CEventQueue(int maxdepth) :
    mBlockingMode(true),
    mShutdown(false),
    mMaxDepth((maxdepth <= EVQ_MAX_DEPTH) ? maxdepth : EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex)
{
    mNotify = NULL;
}

CEventQueue::CEventQueue(int maxdepth, CThreadCondition *notifier) :
    mBlockingMode(true),
    mShutdown(false),
    mMaxDepth((maxdepth <= EVQ_MAX_DEPTH) ? maxdepth : EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex)
{
    mNotify = notifier;
}

CEventQueue::~CEventQueue()
{
    CAutoUnlockMutex lock(mMutex);
    // delete all events in queue
    std::list<CEvent*>::iterator i;
    for (i = mQueue.begin(); i != mQueue.end(); ++i)
    {
        CEvent *e = *i;
        if (e != NULL)
            delete e;
    }
    // kick out anyone waiting for the empty condition
    mEmptyCondition.broadcast();
}

void CEventQueue::add(CEvent *e)
{
    // check for NULL event
    if (e == NULL)
        throw CForteEventQueueException("attempt to add NULL event to queue");
    if (mShutdown)
        throw CForteEventQueueException("unable to add event: queue is shutting down");
    if (mBlockingMode)
        mMaxDepth.wait();
    // XXX race condition
    CAutoUnlockMutex lock(mMutex);
    if (!mBlockingMode && mMaxDepth.trywait() == -1 && errno == EAGAIN)
    {
        // non blocking mode and max depth, delete the oldest entry
        std::list<CEvent*>::iterator i;
        i = mQueue.begin();
        if (i != mQueue.end())
        {
            if (*i != NULL) delete *i;
            mQueue.pop_front();
            mMaxDepth.post();
        }
    }

    mQueue.push_back(e);
    // XXX check for a deep queue
    // signal appropriate threads
    if (mNotify) mNotify->signal();
}

CEvent * CEventQueue::get(void)
{
    CAutoUnlockMutex lock(mMutex);
    std::list<CEvent*>::iterator i;
    i = mQueue.begin();
    if (i == mQueue.end())
        return NULL;
    CEvent *ret = *i;
    mQueue.pop_front();
    mMaxDepth.post();
    if (mQueue.empty())
        mEmptyCondition.broadcast();
    return ret;
}

int CEventQueue::getEventCopies(int maxEvents, std::list<CEvent*> &result)
{
    result.clear();
    CAutoUnlockMutex lock(mMutex);
    std::list<CEvent*>::iterator i;
    int count = 0;
    for (i = mQueue.begin(); i != mQueue.end() && maxEvents-- > 0;
         ++i)
    {
        CEvent *e = *i;
        if (e != NULL)
        {
            result.push_back(e->copy());
            ++count;
        }
    }
    return count;
}
