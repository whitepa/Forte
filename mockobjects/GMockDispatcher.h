#ifndef _GMockDispatcher_h
#define _GMockDispatcher_h

#include "Dispatcher.h"
#include "Event.h"
#include <gmock/gmock.h>
#include <boost/make_shared.hpp>

namespace Forte
{
    class NoOpRequestHandler : public RequestHandler
    {
    public:
        NoOpRequestHandler() {}
        virtual ~NoOpRequestHandler() {}

        void Handler(Event *e) {}
        void Busy(void) {}
        void Periodic(void) {}
        void Init(void) {}
        void Cleanup(void) {}
    };

    class GMockDispatcher : public Dispatcher
    {
    public:
        GMockDispatcher()
            : Dispatcher(
                boost::make_shared<NoOpRequestHandler>(),
                1,
                "GMockDispatcher")
            {}
        ~GMockDispatcher() {}

        MOCK_METHOD0(Pause, void(void));
        MOCK_METHOD0(Resume, void(void));
        MOCK_METHOD1(Enqueue, void(boost::shared_ptr<Event> e));
        MOCK_METHOD0(Accepting, bool(void));
        MOCK_METHOD0(Shutdown, void(void));
        MOCK_METHOD2(GetQueuedEvents,
                     int(int maxEvents,
                         std::list<boost::shared_ptr<Event> > &queuedEvents));
        MOCK_METHOD2(GetRunningEvents,
                     int(int maxEvents,
                         std::list<boost::shared_ptr<Event> > &runningEvents));
    };

    typedef boost::shared_ptr<GMockDispatcher> GMockDispatcherPtr;
};
#endif
