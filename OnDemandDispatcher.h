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
        OnDemandDispatcherManager(OnDemandDispatcher &disp);
        virtual ~OnDemandDispatcherManager() {};
    protected:
        virtual void *run(void);
    };
    class OnDemandDispatcherWorker : public DispatcherThread
    {
    public:
        OnDemandDispatcherWorker(OnDemandDispatcher &disp, shared_ptr<Event> event);
        ~OnDemandDispatcherWorker();
    protected:
        virtual void *run(void);
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
        virtual void enqueue(shared_ptr<Event> e);
        virtual bool accepting(void);

        inline int getQueuedEvents(int maxEvents, 
                                   std::list<shared_ptr<Event> > &queuedEvents)
            { return mEventQueue.getEvents(maxEvents, queuedEvents); }
        int getRunningEvents(int maxEvents, 
                             std::list<shared_ptr<Event> > &runningEvents);

    protected:
        unsigned int mMaxThreads;
        Semaphore mThreadSem;
        OnDemandDispatcherManager mManagerThread;
    };
};

#endif
