#ifndef __OnDemandDispatcher_h
#define __OnDemandDispatcher_h

#include "Dispatcher.h"
#include <boost/scoped_ptr.hpp>

namespace Forte
{
    class OnDemandDispatcher;
    class OnDemandDispatcherManager : public DispatcherThread
    {
    public:
        OnDemandDispatcherManager(OnDemandDispatcher &disp);
        virtual ~OnDemandDispatcherManager() { deleting();};
    protected:
        virtual void *run(void);
    };

    class OnDemandDispatcherWorker : public DispatcherThread
    {
    public:
        OnDemandDispatcherWorker(
            OnDemandDispatcher &disp,
            boost::shared_ptr<Event> event);

        virtual ~OnDemandDispatcherWorker();

    protected:
        virtual void *run(void);
    };

    class OnDemandDispatcher : public Dispatcher
    {
        friend class OnDemandDispatcherManager;
        friend class OnDemandDispatcherWorker;
    public:
        OnDemandDispatcher(
            boost::shared_ptr<RequestHandler> requestHandler,
            const int maxThreads,
            const int deepQueue,
            const int maxDepth,
            const char *name);
        virtual ~OnDemandDispatcher();
        virtual void Shutdown();
        virtual void Pause(void);
        virtual void Resume(void);
        virtual void Enqueue(boost::shared_ptr<Event> e);
        virtual bool Accepting(void);

        inline int GetQueuedEvents(
            int maxEvents,
            std::list<boost::shared_ptr<Event> > &queuedEvents)
            {
                return mEventQueue.GetEvents(maxEvents, queuedEvents);
            }
        int GetRunningEvents(int maxEvents,
                             std::list<boost::shared_ptr<Event> > &runningEvents);
        bool StopRunningEvent(boost::shared_ptr<Event> &runningEvent);

    protected:
        unsigned int mMaxThreads;
        Semaphore mThreadSem;
        OnDemandDispatcherManager mManagerThread;
    };;
};

#endif
