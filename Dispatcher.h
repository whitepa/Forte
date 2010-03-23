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

namespace Forte
{
    class DispatcherThread;
    class Dispatcher : public Object
    {
    public:
        Dispatcher(RequestHandler &reqHandler, int maxQueueDepth, const char *name);
        virtual ~Dispatcher();

        virtual void pause(void) = 0;
        virtual void resume(void) = 0;

        virtual void enqueue(Event *e) = 0;
        virtual bool accepting(void) = 0;
        virtual int getQueuedEvents(int maxEvents, std::list<Event*> &queuedEvents) = 0;
        virtual int getRunningEvents(int maxEvents, std::list<Event*> &runningEvents) = 0;
    
        void registerThread(DispatcherThread *thr);
        void unregisterThread(DispatcherThread *thr);
        FString mDispatcherName;
    protected:
        bool mPaused;
        volatile bool mShutdown;
        std::vector<shared_ptr<DispatcherThread> > mThreads;
        RequestHandler &mRequestHandler;
        Mutex mThreadsLock;
        Mutex mNotifyLock;
        ThreadCondition mNotify;
        EventQueue mEventQueue;
    };


    class DispatcherThread : public Thread
    {
        friend class ThreadPoolDispatcher;
        friend class OnDemandDispatcher;
    public:
        DispatcherThread(Dispatcher &dispatcher);
        virtual ~DispatcherThread();

        /**
         * HasEvent() returns whether this thread is currently
         * assigned an event.
         */
        bool HasEvent(void) { return mEventPtr; }

    protected:
        Dispatcher &mDispatcher;
        // pointer to the event being handled by this thread, NULL if idle.
        // YOU MUST LOCK dispatcher's mNotifyMutex while accessing this data
        shared_ptr<Event> mEventPtr;
    };
};


#endif
