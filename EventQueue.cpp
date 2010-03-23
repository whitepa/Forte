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
    // delete all events in queue
    std::list<Event*>::iterator i;
    for (i = mQueue.begin(); i != mQueue.end(); ++i)
    {
        Event *e = *i;
        if (e != NULL)
            delete e;
    }
    // kick out anyone waiting for the empty condition
    mEmptyCondition.broadcast();
}

void EventQueue::add(Event *e)
{
    // check for NULL event
    if (e == NULL)
        throw ForteEventQueueException("attempt to add NULL event to queue");
    if (mShutdown)
        throw ForteEventQueueException("unable to add event: queue is shutting down");
    if (mBlockingMode)
        mMaxDepth.wait();
    // XXX race condition
    AutoUnlockMutex lock(mMutex);
    if (!mBlockingMode && mMaxDepth.trywait() == -1 && errno == EAGAIN)
    {
        // non blocking mode and max depth, delete the oldest entry
        std::list<Event*>::iterator i;
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

Event * EventQueue::get(void)
{
    AutoUnlockMutex lock(mMutex);
    std::list<Event*>::iterator i;
    i = mQueue.begin();
    if (i == mQueue.end())
        return NULL;
    Event *ret = *i;
    mQueue.pop_front();
    mMaxDepth.post();
    if (mQueue.empty())
        mEmptyCondition.broadcast();
    return ret;
}

int EventQueue::getEventCopies(int maxEvents, std::list<Event*> &result)
{
    result.clear();
    AutoUnlockMutex lock(mMutex);
    std::list<Event*>::iterator i;
    int count = 0;
    for (i = mQueue.begin(); i != mQueue.end() && maxEvents-- > 0;
         ++i)
    {
        Event *e = *i;
        if (e != NULL)
        {
            result.push_back(e->copy());
            ++count;
        }
    }
    return count;
}
