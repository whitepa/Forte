#ifndef __ThreadPoolDispatcher_h
#define __ThreadPoolDispatcher_h

#include "Dispatcher.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, ForteThreadPoolDispatcherException);

    class ThreadPoolDispatcher;
    class ThreadPoolDispatcherManager : public DispatcherThread
    {
    public:
        ThreadPoolDispatcherManager(ThreadPoolDispatcher &disp);
        virtual ~ThreadPoolDispatcherManager() { deleting(); };
    protected:
        virtual void *run(void);
    };
    class ThreadPoolDispatcherWorker : public DispatcherThread
    {
    public:
        ThreadPoolDispatcherWorker(ThreadPoolDispatcher &disp);
        virtual ~ThreadPoolDispatcherWorker();
    protected:
        virtual void *run(void);
        MonotonicClock mMonotonicClock;
        time_t mLastPeriodicCall;
    };
    class ThreadPoolDispatcher : public Dispatcher
    {
        friend class ThreadPoolDispatcherManager;
        friend class ThreadPoolDispatcherWorker;
    public:
        ThreadPoolDispatcher(boost::shared_ptr<RequestHandler> requestHandler,
                              const int minThreads, const int maxThreads, 
                              const int minSpareThreads, const int maxSpareThreads,
                              const int deepQueue, const int maxDepth, const char *name);
        virtual ~ThreadPoolDispatcher();
        virtual void Pause(void);
        virtual void Resume(void);
        virtual void Enqueue(shared_ptr<Event> e);
        virtual bool Accepting(void);

        inline int GetQueuedEvents(int maxEvents, std::list<shared_ptr<Event> > &queuedEvents)
            { return mEventQueue.GetEvents(maxEvents, queuedEvents); }
        int GetRunningEvents(int maxEvents, std::list<shared_ptr<Event> > &runningEvents);

    protected:
        unsigned int mMinThreads;
        unsigned int mMaxThreads;
        unsigned int mMinSpareThreads;
        unsigned int mMaxSpareThreads;
        Semaphore mThreadSem;
        Semaphore mSpareThreadSem;
        ThreadPoolDispatcherManager mManagerThread;
    };;
};
#endif
