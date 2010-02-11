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
    EXCEPTION_SUBCLASS(CForteException, CForteThreadPoolDispatcherException);

    class CDispatcherThread;
    class CDispatcher
    {
    public:
        CDispatcher();
        virtual ~CDispatcher();

        virtual void pause(void) = 0;
        virtual void resume(void) = 0;

        virtual void enqueue(CEvent *e) = 0;
        virtual bool accepting(void) = 0;
        virtual int getQueuedEvents(int maxEvents, std::list<CEvent*> &queuedEvents) = 0;
        virtual int getRunningEvents(int maxEvents, std::list<CEvent*> &runningEvents) = 0;
    
        void registerThread(CDispatcherThread *thr);
        void unregisterThread(CDispatcherThread *thr);
        FString mDispatcherName;
    protected:
        bool mPaused;
        volatile bool mShutdown;
        std::vector<CDispatcherThread*> mThreads;
        CMutex mThreadsLock;
    };
    class CDispatcherThread : public CThread
    {
        friend class CThreadPoolDispatcher;
        friend class COnDemandDispatcher;
    public:
        CDispatcherThread();
        virtual ~CDispatcherThread();

    protected:
        CDispatcher *mDispatcher;
        bool mRegistered;
        // pointer to the event being handled by this thread, NULL if idle.
        // YOU MUST LOCK dispatcher's mNotifyMutex while accessing this data
        CEvent *mEventPtr;
    };
    class CThreadPoolDispatcher;
    class CThreadPoolDispatcherManager : public CDispatcherThread
    {
    public:
        inline CThreadPoolDispatcherManager(CThreadPoolDispatcher *disp)
            {mDispatcher = (CDispatcher*)disp; initialized();};
    protected:
        virtual void *run(void);
    };
    class CThreadPoolDispatcherWorker : public CDispatcherThread
    {
    public:
        CThreadPoolDispatcherWorker(CThreadPoolDispatcher *disp);
        virtual ~CThreadPoolDispatcherWorker();
    protected:
        virtual void *run(void);
        time_t mLastPeriodicCall;
    };
    class CThreadPoolDispatcher : public CDispatcher
    {
        friend class CThreadPoolDispatcherManager;
        friend class CThreadPoolDispatcherWorker;
    public:
        CThreadPoolDispatcher(CRequestHandler &requestHandler,
                              const int minThreads, const int maxThreads, 
                              const int minSpareThreads, const int maxSpareThreads,
                              const int deepQueue, const int maxDepth, const char *name);
        virtual ~CThreadPoolDispatcher();
        virtual void pause(void);
        virtual void resume(void);
        virtual void enqueue(CEvent *e);
        virtual bool accepting(void);

        inline int getQueuedEvents(int maxEvents, std::list<CEvent*> &queuedEvents)
            { return mEventQueue.getEventCopies(maxEvents, queuedEvents); }
        int getRunningEvents(int maxEvents, std::list<CEvent*> &runningEvents);

    protected:
        CRequestHandler &mRequestHandler;
        unsigned int mMinThreads;
        unsigned int mMaxThreads;
        unsigned int mMinSpareThreads;
        unsigned int mMaxSpareThreads;
        CSemaphore mThreadSem;
        CSemaphore mSpareThreadSem;
        CMutex mNotifyLock;
        CThreadCondition mNotify;
        CEventQueue mEventQueue;
        CThreadPoolDispatcherManager mManagerThread;
    };

    class COnDemandDispatcher;
    class COnDemandDispatcherManager : public CDispatcherThread
    {
    public:
        inline COnDemandDispatcherManager(COnDemandDispatcher *disp)
            {mDispatcher = (CDispatcher*)disp; initialized();};
    protected:
        virtual void *run(void);
    };
    class COnDemandDispatcherWorker : public CDispatcherThread
    {
    public:
        COnDemandDispatcherWorker(COnDemandDispatcher *disp, CEvent *event);
        ~COnDemandDispatcherWorker();
    protected:
        virtual void *run(void);
        auto_ptr<CEvent> mEvent;
    };


    class COnDemandDispatcher : public CDispatcher
    {
        friend class COnDemandDispatcherManager;
        friend class COnDemandDispatcherWorker;
    public:
        COnDemandDispatcher(CRequestHandler &requestHandler,
                            const int maxThreads,
                            const int deepQueue, const int maxDepth, const char *name);
        virtual ~COnDemandDispatcher();
        virtual void pause(void);
        virtual void resume(void);
        virtual void enqueue(CEvent *e);
        virtual bool accepting(void);

        inline int getQueuedEvents(int maxEvents, std::list<CEvent*> &queuedEvents)
            { return mEventQueue.getEventCopies(maxEvents, queuedEvents); }
        int getRunningEvents(int maxEvents, std::list<CEvent*> &runningEvents);

    protected:
        CRequestHandler &mRequestHandler;
        unsigned int mMaxThreads;
        CSemaphore mThreadSem;
        CMutex mNotifyLock;
        CThreadCondition mNotify;
        CEventQueue mEventQueue;
        COnDemandDispatcherManager mManagerThread;
    };
};
#endif
