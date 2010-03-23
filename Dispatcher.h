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
        Dispatcher();
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
        std::vector<DispatcherThread*> mThreads;
        Mutex mThreadsLock;
    };
    class DispatcherThread : public Thread
    {
        friend class ThreadPoolDispatcher;
        friend class OnDemandDispatcher;
    public:
        DispatcherThread();
        virtual ~DispatcherThread();

    protected:
        Dispatcher *mDispatcher;
        bool mRegistered;
        // pointer to the event being handled by this thread, NULL if idle.
        // YOU MUST LOCK dispatcher's mNotifyMutex while accessing this data
        Event *mEventPtr;
    };
};


#endif
