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
    EXCEPTION_SUBCLASS(ForteException, ForteThreadPoolDispatcherException);

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
    class ThreadPoolDispatcher;
    class ThreadPoolDispatcherManager : public DispatcherThread
    {
    public:
        inline ThreadPoolDispatcherManager(ThreadPoolDispatcher *disp)
            {mDispatcher = (Dispatcher*)disp; initialized();};
    protected:
        virtual void *run(void);
    };
    class ThreadPoolDispatcherWorker : public DispatcherThread
    {
    public:
        ThreadPoolDispatcherWorker(ThreadPoolDispatcher *disp);
        virtual ~ThreadPoolDispatcherWorker();
    protected:
        virtual void *run(void);
        time_t mLastPeriodicCall;
    };
    class ThreadPoolDispatcher : public Dispatcher
    {
        friend class ThreadPoolDispatcherManager;
        friend class ThreadPoolDispatcherWorker;
    public:
        ThreadPoolDispatcher(RequestHandler &requestHandler,
                              const int minThreads, const int maxThreads, 
                              const int minSpareThreads, const int maxSpareThreads,
                              const int deepQueue, const int maxDepth, const char *name);
        virtual ~ThreadPoolDispatcher();
        virtual void pause(void);
        virtual void resume(void);
        virtual void enqueue(Event *e);
        virtual bool accepting(void);

        inline int getQueuedEvents(int maxEvents, std::list<Event*> &queuedEvents)
            { return mEventQueue.getEventCopies(maxEvents, queuedEvents); }
        int getRunningEvents(int maxEvents, std::list<Event*> &runningEvents);

    protected:
        RequestHandler &mRequestHandler;
        unsigned int mMinThreads;
        unsigned int mMaxThreads;
        unsigned int mMinSpareThreads;
        unsigned int mMaxSpareThreads;
        Semaphore mThreadSem;
        Semaphore mSpareThreadSem;
        Mutex mNotifyLock;
        ThreadCondition mNotify;
        EventQueue mEventQueue;
        ThreadPoolDispatcherManager mManagerThread;
    };

    class OnDemandDispatcher;
    class OnDemandDispatcherManager : public DispatcherThread
    {
    public:
        inline OnDemandDispatcherManager(OnDemandDispatcher *disp)
            {mDispatcher = (Dispatcher*)disp; initialized();};
    protected:
        virtual void *run(void);
    };
    class OnDemandDispatcherWorker : public DispatcherThread
    {
    public:
        OnDemandDispatcherWorker(OnDemandDispatcher *disp, Event *event);
        ~OnDemandDispatcherWorker();
    protected:
        virtual void *run(void);
        auto_ptr<Event> mEvent;
    };


    class OnDemandDispatcher : public Dispatcher
    {
        friend class OnDemandDispatcherManager;
        friend class OnDemandDispatcherWorker;
    public:
        OnDemandDispatcher(RequestHandler &requestHandler,
                            const int maxThreads,
                            const int deepQueue, const int maxDepth, const char *name);
        virtual ~OnDemandDispatcher();
        virtual void pause(void);
        virtual void resume(void);
        virtual void enqueue(Event *e);
        virtual bool accepting(void);

        inline int getQueuedEvents(int maxEvents, std::list<Event*> &queuedEvents)
            { return mEventQueue.getEventCopies(maxEvents, queuedEvents); }
        int getRunningEvents(int maxEvents, std::list<Event*> &runningEvents);

    protected:
        RequestHandler &mRequestHandler;
        unsigned int mMaxThreads;
        Semaphore mThreadSem;
        Mutex mNotifyLock;
        ThreadCondition mNotify;
        EventQueue mEventQueue;
        OnDemandDispatcherManager mManagerThread;
    };
};
#endif
