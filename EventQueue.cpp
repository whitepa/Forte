#include "Forte.h"
#define EVQ_MAX_DEPTH 65536

EventQueue::EventQueue() :
    mBlockingMode(true),
    mShutdown(false),
    mMaxDepth(EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex)
{
    mNotify = NULL;
}

EventQueue::EventQueue(int maxdepth) :
    mBlockingMode(true),
    mShutdown(false),
    mMaxDepth((maxdepth <= EVQ_MAX_DEPTH) ? maxdepth : EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex)
{
    mNotify = NULL;
}

EventQueue::EventQueue(int maxdepth, ThreadCondition *notifier) :
    mBlockingMode(true),
    mShutdown(false),
    mMaxDepth((maxdepth <= EVQ_MAX_DEPTH) ? maxdepth : EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex)
{
    mNotify = notifier;
}

EventQueue::~EventQueue()
{
    AutoUnlockMutex lock(mMutex);
    // cleanup the queue
    mQueue.clear();
    // kick out anyone waiting for the empty condition
    mEmptyCondition.Broadcast();
}

void EventQueue::Add(shared_ptr<Event> e)
{
    // check for NULL event
    if (!e)
        throw ForteEventQueueException("attempt to add NULL event to queue");
    if (mShutdown)
        throw ForteEventQueueException("unable to add event: queue is shutting down");
    if (mBlockingMode)
        mMaxDepth.Wait();
    // XXX race condition
    AutoUnlockMutex lock(mMutex);
    if (!mBlockingMode && mMaxDepth.TryWait() == -1 && errno == EAGAIN)
    {
        // non blocking mode and max depth, delete the oldest entry
        std::list<shared_ptr<Event> >::iterator i;
        i = mQueue.begin();
        if (i != mQueue.end())
        {
            mQueue.pop_front();
            mMaxDepth.Post();
        }
    }
    mQueue.push_back(e);
    // XXX check for a deep queue
    // signal appropriate threads
    if (mNotify) mNotify->Signal();
}

shared_ptr<Event> EventQueue::Get(void)
{
    AutoUnlockMutex lock(mMutex);
    shared_ptr<Event> e;
    std::list<shared_ptr<Event> >::iterator i;
    i = mQueue.begin();
    if (i == mQueue.end())
        return e;
    e = *i;
    mQueue.pop_front();
    mMaxDepth.Post();
    if (mQueue.empty())
        mEmptyCondition.Broadcast();
    return e;
}

int EventQueue::GetEvents(int maxEvents, std::list<shared_ptr<Event> > &result)
{
    result.clear();
    AutoUnlockMutex lock(mMutex);
    std::list<shared_ptr<Event> >::iterator i;
    int count = 0;
    for (i = mQueue.begin(); i != mQueue.end() && maxEvents-- > 0;
         ++i)
    {
        shared_ptr<Event> e = *i;
        if (e)
        {
            result.push_back(e);
            ++count;
        }
    }
    return count;
}
