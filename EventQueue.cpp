#include "EventQueue.h"
#include "LogManager.h"

using namespace boost;
using namespace Forte;

#define EVQ_MAX_DEPTH 65536

EventQueue::EventQueue(QueueMode mode) :
    mMode(mode),
    mShutdown(false),
    mMaxDepth(EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex),
    mNotifyMutex(NULL),
    mNotify(NULL)
{
}

EventQueue::EventQueue(int maxdepth, QueueMode mode) :
    mMode(mode),
    mShutdown(false),
    mMaxDepth((maxdepth <= EVQ_MAX_DEPTH) ? maxdepth : EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex),
    mNotifyMutex(NULL),
    mNotify(NULL)
{
}

EventQueue::EventQueue(int maxdepth,
                       Mutex* notifierMutex,
                       ThreadCondition* notifier,
                       QueueMode mode) :
    mMode(mode),
    mShutdown(false),
    mMaxDepth((maxdepth <= EVQ_MAX_DEPTH) ? maxdepth : EVQ_MAX_DEPTH),
    mEmptyCondition(mMutex),
    mNotifyMutex(notifierMutex),
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

void EventQueue::Add(EventQueue::EventPtr e)
{
    // check for NULL event
    if (!e)
        throw EEventQueueEventInvalid();
    if (mMode == QUEUE_MODE_BLOCKING)
        mMaxDepth.Wait();
    {
        AutoUnlockMutex lock(mMutex);
        if (mShutdown)
            throw EEventQueueShutdown();
        // \TODO fix this race condition
        if (mMode != QUEUE_MODE_BLOCKING
            && mMaxDepth.TryWait() == -1
            && errno == EAGAIN)
        {
            // max depth
            if (mMode == QUEUE_MODE_DROP_OLDEST || mMode == QUEUE_MODE_DROP_OLDEST_LOG)
            {
                // delete the oldest entry
                std::list<EventQueue::EventPtr >::iterator i;
                i = mQueue.begin();
                if (i != mQueue.end())
                {
                    if (mMode == QUEUE_MODE_DROP_OLDEST_LOG && (*i))
                        hlog(HLOG_INFO, "Event queue full: dropping oldest event (%s)", (*i)->mName.c_str());
                    mQueue.pop_front();
                }
            }
            else if (mMode == QUEUE_MODE_THROW)
                throw EEventQueueFull();
            else
                throw EEventQueue(FStringFC(), "invalid queue mode %d", mMode);
        }
        mQueue.push_back(e);
    }

    // \TODO check for a deep queue, and warn

    // signal appropriate threads
    if (mNotify)
    {
        AutoUnlockMutex lock(*mNotifyMutex);
        mNotify->Signal();
    }
}

EventQueue::EventPtr EventQueue::Get(void)
{
    std::vector<EventQueue::EventPtr > events(
        Get(1));

    if(events.empty())
    {
        return EventQueue::EventPtr();
    }
    else
    {
        return *events.begin();
    }
}

EventQueue::EventPtrVector EventQueue::Get(const unsigned long& max)
{
    AutoUnlockMutex lock(mMutex);
    std::vector<EventQueue::EventPtr > events;
    std::list<EventQueue::EventPtr >::iterator i;

    for(i = mQueue.begin();
        (i != mQueue.end()) && (events.size() < max);
        i = mQueue.begin())
    {
        events.push_back(*i);
        mQueue.pop_front();
    }

    if (events.empty())
    {
        return events;
    }

    mMaxDepth.Post(events.size());
    if (mQueue.empty())
        mEmptyCondition.Broadcast();
    return events;
}

EventQueue::EventPtr EventQueue::Peek(void)
{
    AutoUnlockMutex lock(mMutex);
    EventQueue::EventPtr e;
    std::list<EventQueue::EventPtr >::iterator i;
    i = mQueue.begin();
    if (i == mQueue.end())
        return e;
    e = *i;
    return e;
}

int EventQueue::GetEvents(int maxEvents, std::list<EventQueue::EventPtr > &result)
{
    result.clear();
    AutoUnlockMutex lock(mMutex);
    std::list<EventQueue::EventPtr >::iterator i;
    int count = 0;
    for (i = mQueue.begin(); i != mQueue.end() && maxEvents-- > 0;
         ++i)
    {
        EventQueue::EventPtr e = *i;
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
