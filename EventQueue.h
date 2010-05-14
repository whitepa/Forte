#ifndef _Event_Queue_h
#define _Event_Queue_h

#include "Semaphore.h"
#include "Event.h"
#include "Exception.h"
#include <list>
#include <boost/shared_ptr.hpp>

using namespace boost;

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, ForteEventQueueException);

    class EventQueue : public Object
    {
    public:
        EventQueue();
        EventQueue(int maxdepth);
        EventQueue(int maxdepth, ThreadCondition *notifier);
        virtual ~EventQueue();

        void Add(shared_ptr<Event> e);
        shared_ptr<Event> Get(void);
        inline bool Accepting(void) { return (!mShutdown && ((mMaxDepth.GetValue() > 0) ? true : false));}
        inline int Depth(void) {AutoUnlockMutex lock(mMutex); return mQueue.size();};

        /// set to true (default) to block on full queue.
        /// set to false to drop old events (and delete them) on full queue
        inline void SetBlockingMode(bool blocking) { mBlockingMode = blocking; };

        /// shutdown prevents the queue from accepting any more events via Add().
        /// This operation is (currently) not reversible.
        inline void Shutdown(void) { mShutdown = true; };

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

        /// getEventCopies retreives references of the next maxEvents in the queue
        ///
        int GetEvents(int maxEvents, std::list<shared_ptr<Event> > &result);

        int mDeepThresh;
        int mLastDepth;
    protected:
        bool mBlockingMode;
        bool mShutdown;
        std::list<shared_ptr<Event> > mQueue;
        Semaphore mMaxDepth;
        Mutex mMutex;
        ThreadCondition mEmptyCondition;
        ThreadCondition *mNotify;
    };
};
#endif
