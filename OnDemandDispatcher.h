#ifndef __OnDemandDispatcher_h
#define __OnDemandDispatcher_h

#include "Dispatcher.h"
#include <boost/scoped_ptr.hpp>
using namespace boost;

namespace Forte
{
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
        scoped_ptr<Event> mEvent;
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
