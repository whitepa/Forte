#ifndef __Dispatcher_h
#define __Dispatcher_h

#include "Event.h"
#include "EventQueue.h"
#include "RequestHandler.h"
#include "AutoMutex.h"
#include "ThreadCondition.h"
#include "Semaphore.h"
#include "Thread.h"
#include <vector>
#include <memory>

/**
 *
 * The dispatcher class keeps a shared pointer to its request handler.
 *
 * If the context contains shared pointers to both, that is
 * acceptable, as it will not create a reference loop.
 *
 * RECEIVER -->  *DISPATCHER*  -->  REQHANDLER
 *
 */

namespace Forte
{
    EXCEPTION_CLASS(EDispatcher);
    EXCEPTION_SUBCLASS2(EDispatcher,
                        EDispatcherReqHandlerInvalid,
                        "Request handler is invalid");

    class DispatcherThread;
    class DispatcherWorkerThread;

    class Dispatcher : public Object
    {
    public:
        Dispatcher(boost::shared_ptr<RequestHandler> reqHandler,
                   int maxQueueDepth, const char *name);
        virtual ~Dispatcher();

        virtual void Pause(void) = 0;
        virtual void Resume(void) = 0;

        virtual void Enqueue(boost::shared_ptr<Event> e) = 0;
        virtual bool Accepting(void) = 0;
        virtual int GetQueuedEvents(
            int maxEvents,
            std::list<boost::shared_ptr<Event> > &queuedEvents) = 0;
        virtual int GetRunningEvents(
            int maxEvents,
            std::list<boost::shared_ptr<Event> > &runningEvents) = 0;

        virtual int GetQueueDepth(void) { return mEventQueue.Depth(); }

        virtual int GetThreadCount(void) {
            AutoUnlockMutex lock(mThreadsLock);
            return mThreads.size();
        }

        virtual void Shutdown();
        bool IsShuttingDown() {
            AutoUnlockMutex lock(mNotifyLock);
            return mShutdown;
        }

        FString mDispatcherName;

    protected:
        bool mPaused;
        bool mShutdown;
        std::vector<boost::shared_ptr<DispatcherWorkerThread> > mThreads;
        boost::shared_ptr<RequestHandler> mRequestHandler;
        Mutex mThreadsLock;
        Mutex mNotifyLock;
        ThreadCondition mNotify;
        EventQueue mEventQueue;
    };
    typedef boost::shared_ptr<Dispatcher> DispatcherPtr;

    class DispatcherThread : public Thread
    {
        friend class ThreadPoolDispatcher;
        friend class OnDemandDispatcher;
    public:
        DispatcherThread(Dispatcher &dispatcher);
        virtual ~DispatcherThread();

    protected:
        Dispatcher &mDispatcher;
    };

    class DispatcherWorkerThread : public DispatcherThread
    {
        friend class ThreadPoolDispatcher;
        friend class OnDemandDispatcher;
    public:
        DispatcherWorkerThread(Dispatcher &dispatcher,
                               const boost::shared_ptr<Event>& e);
        virtual ~DispatcherWorkerThread();

        /**
         * HasEvent() returns whether this thread is currently
         * assigned an event.
         */
        bool HasEvent(void) {
            AutoUnlockMutex lock(mEventMutex);
            return mEventPtr;
        }

    protected:
        void setEvent(const boost::shared_ptr<Event>& event) {
            AutoUnlockMutex lock(mEventMutex);
            mEventPtr = event;
        }

        void clearEvent() {
            AutoUnlockMutex lock(mEventMutex);
            mEventPtr.reset();
        }

        Event* getRawEventPointer() {
            AutoUnlockMutex lock(mEventMutex);
            return mEventPtr.get(); // modeling current usage
        }

        boost::shared_ptr<Event> getEvent() {
            return mEventPtr;
        }


    private:
        // a very small lock to protect just the event
        Forte::Mutex mEventMutex;

        // pointer to the event being handled by this thread, NULL if idle.
        // YOU MUST LOCK dispatcher's mNotifyMutex while accessing this data
        boost::shared_ptr<Event> mEventPtr;
    };
};


#endif
