#ifndef _Event_Queue_h
#define _Event_Queue_h

#include "Semaphore.h"
#include "Event.h"
#include "Exception.h"
#include <list>

namespace Forte
{
    EXCEPTION_SUBCLASS(CForteException, CForteEventQueueException);

    class CEventQueue : public Object
    {
    public:
        CEventQueue();
        CEventQueue(int maxdepth);
        CEventQueue(int maxdepth, CThreadCondition *notifier);
        virtual ~CEventQueue();

        void add(CEvent *e);
        CEvent * get(void);
        inline bool accepting(void) { return (!mShutdown && ((mMaxDepth.getvalue() > 0) ? true : false));}
        inline int depth(void) {CAutoUnlockMutex lock(mMutex); return mQueue.size();};

        /// set to true (default) to block on full queue.
        /// set to false to drop old events (and delete them) on full queue
        inline void setBlockingMode(bool blocking) { mBlockingMode = blocking; };

        /// shutdown prevents the queue from accepting any more events via add().
        /// This operation is (currently) not reversible.
        inline void shutdown(void) { mShutdown = true; };

        /// waitUntilEmpty will cause the caller to block until the event
        /// queue is emptied.  If the queue is already empty upon calling
        /// waitUntilEmpty(), it will return immediately.
        inline void waitUntilEmpty(void) { 
            CAutoUnlockMutex lock(mMutex); 
            if (mQueue.empty())
                return;
            else
                mEmptyCondition.wait();
        };

        /// getEventCopies retreives copies of the next maxEvents in the queue
        ///
        int getEventCopies(int maxEvents, std::list<CEvent*> &result);

        int mDeepThresh;
        int mLastDepth;
    protected:
        bool mBlockingMode;
        bool mShutdown;
        std::list<CEvent*> mQueue;
        CSemaphore mMaxDepth;
        CMutex mMutex;
        CThreadCondition mEmptyCondition;
        CThreadCondition *mNotify;
    };
};
#endif
