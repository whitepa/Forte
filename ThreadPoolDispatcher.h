#ifndef __ThreadPoolDispatcher_h
#define __ThreadPoolDispatcher_h

#include "Dispatcher.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(ForteException, ForteThreadPoolDispatcherException);

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
};
#endif
