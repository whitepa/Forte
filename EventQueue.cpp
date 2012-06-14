#include "EventQueue.h"
#include "LogManager.h"

using namespace Forte;

#define EVQ_MAX_DEPTH 65536

EventQueue::EventQueue(QueueMode mode) :
    mMode(mode),
    mShutdown(false),
    mMaxDepth(EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex),
    mNotify(NULL)
{
}

EventQueue::EventQueue(int maxdepth, QueueMode mode) :
    mMode(mode),
    mShutdown(false),
    mMaxDepth((maxdepth <= EVQ_MAX_DEPTH) ? maxdepth : EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex),
    mNotify(NULL)
{
}

EventQueue::EventQueue(int maxdepth, ThreadCondition *notifier, QueueMode mode) :
    mMode(mode),
    mShutdown(false),
    mMaxDepth((maxdepth <= EVQ_MAX_DEPTH) ? maxdepth : EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex),
    mNotify(notifier)
{
}

EventQueue::~EventQueue()
{
    AutoUnlockMutex lock(mMutex);
    // set shutdown flag
    mShutdown = true;
    // cleanup the queue
    mQueue.clear();
    // kick out anyone waiting for the empty condition
    mEmptyCondition.Broadcast();
}

void EventQueue::Add(shared_ptr<Event> e)
{
    // check for NULL event
    if (!e)
        throw EEventQueueEventInvalid();
    if (mShutdown)
        throw EEventQueueShutdown();
    if (mMode == QUEUE_MODE_BLOCKING)
        mMaxDepth.Wait();
    // \TODO fix this race condition
    AutoUnlockMutex lock(mMutex);
    if (mMode != QUEUE_MODE_BLOCKING && mMaxDepth.TryWait() == -1 && errno == EAGAIN)
    {
        // max depth
        if (mMode == QUEUE_MODE_DROP_OLDEST || mMode == QUEUE_MODE_DROP_OLDEST_LOG)
        {
            // delete the oldest entry
            std::list<shared_ptr<Event> >::iterator i;
            i = mQueue.begin();
            if (i != mQueue.end())
            {
                if (mMode == QUEUE_MODE_DROP_OLDEST_LOG && (*i))
                    hlog(HLOG_INFO, "Event queue full: dropping oldest event (%s)", (*i)->mName.c_str());
                mQueue.pop_front();
                mMaxDepth.Post();
            }
        }
        else if (mMode == QUEUE_MODE_THROW)
            throw EEventQueueFull();
        else
            throw EEventQueue(FStringFC(), "invalid queue mode %d", mMode);
    }
    mQueue.push_back(e);
    // \TODO check for a deep queue, and warn
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
    if (mMode == QUEUE_MODE_BLOCKING)
        mMaxDepth.Post();
    if (mQueue.empty())
        mEmptyCondition.Broadcast();
    return e;
}

shared_ptr<Event> EventQueue::Peek(void)
{
    AutoUnlockMutex lock(mMutex);
    shared_ptr<Event> e;
    std::list<shared_ptr<Event> >::iterator i;
    i = mQueue.begin();
    if (i == mQueue.end())
        return e;
    e = *i;
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

void EventQueue::Clear(void)
{
    AutoUnlockMutex lock(mMutex);
    mMaxDepth.Post(mQueue.size());
    mQueue.clear();
    mEmptyCondition.Broadcast();
}
