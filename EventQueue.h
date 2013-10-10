#ifndef _Event_Queue_h
#define _Event_Queue_h

#include "Semaphore.h"
#include "Event.h"
#include "Exception.h"
#include <list>
#include <boost/shared_ptr.hpp>

namespace Forte
{
    EXCEPTION_CLASS(EEventQueue);
    EXCEPTION_SUBCLASS2(EEventQueue, EEventQueueShutdown, "Event queue is shutting down");
    EXCEPTION_SUBCLASS2(EEventQueue, EEventQueueEventInvalid, "Invalid Event");
    EXCEPTION_SUBCLASS2(EEventQueue, EEventQueueFull, "Event queue is full");

    class EventQueue : public Object
    {
    public:
        typedef boost::shared_ptr<Event> EventPtr;
        typedef std::vector<EventPtr> EventPtrVector;
        enum QueueMode
        {
            QUEUE_MODE_BLOCKING = 0,
            QUEUE_MODE_DROP_OLDEST,
            QUEUE_MODE_DROP_OLDEST_LOG,
            QUEUE_MODE_THROW
        };

        EventQueue(QueueMode mode = QUEUE_MODE_BLOCKING);
        EventQueue(int maxdepth, QueueMode mode = QUEUE_MODE_BLOCKING);
        EventQueue(
            int maxdepth,
            Mutex* notifierMutex,
            ThreadCondition* notifier,
            QueueMode mode = QUEUE_MODE_BLOCKING);
        virtual ~EventQueue();
        void Add(EventPtr e);
        EventPtr Get(void);
        EventPtrVector Get(const unsigned long& max);
        EventPtr Peek(void);
        inline bool Accepting(void) {
            AutoUnlockMutex lock(mMutex);
            return (!mShutdown && ((mMaxDepth.GetValue() > 0) ? true : false));
        }
        inline int Depth(void) {AutoUnlockMutex lock(mMutex); return mQueue.size();};

        /// shutdown prevents the queue from accepting any more events via Add().
        /// This operation is (currently) not reversible.
        inline void Shutdown(void) {
            AutoUnlockMutex lock(mMutex);
            mShutdown = true;
        }

        /// waitUntilEmpty will cause the caller to block until the event
        /// queue is emptied.  If the queue is already empty upon calling
        /// waitUntilEmpty(), it will return immediately.
        inline void WaitUntilEmpty(void) {
            AutoUnlockMutex lock(mMutex);
            if (mQueue.empty())
                return;
            else
                mEmptyCondition.Wait();
         };

        void Clear(void);

        /// getEventCopies retreives references of the next maxEvents in the queue
        ///
        int GetEvents(
            int maxEvents, std::list<EventPtr > &result);

        int mDeepThresh;
        int mLastDepth;

    protected:
        const QueueMode mMode;
        bool mShutdown;
        std::list<EventPtr > mQueue;
        Semaphore mMaxDepth;
        Mutex mMutex;
        ThreadCondition mEmptyCondition;
        Mutex* mNotifyMutex;
        ThreadCondition* mNotify;
    };
};
#endif
