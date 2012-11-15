#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"

#include "RequestHandler.h"
#include "ThreadPoolDispatcher.h"

using namespace std;
using namespace boost;
using namespace Forte;

LogManager logManager;

class TestEvent : public Forte::Event
{
public:
    TestEvent() {}
    virtual ~TestEvent() {}

    virtual void DoWork() = 0;
};

class TestEventDoOutput : public TestEvent
{
public:
    TestEventDoOutput() {}
    virtual ~TestEventDoOutput() {}

    virtual void DoWork() {
        cout << "TestEvent" << endl;
    }
};

class TestEventWorkUntilSignaled : public TestEvent
{
public:
    TestEventWorkUntilSignaled() {}
    virtual ~TestEventWorkUntilSignaled() {}

    virtual void DoWork() {
        while (!Thread::MyThread()->IsShuttingDown())
        {
            const int timeoutInSeconds(5);
            const bool throwOnShutdown(false);
            Thread::InterruptibleSleep(timeoutInSeconds, throwOnShutdown);
        }
    }
};

class ReenqueueingEvent : public TestEvent
{
public:
    ReenqueueingEvent(DispatcherPtr dispatcher)
        : mDispatcher(dispatcher)
        {
        }

    virtual ~ReenqueueingEvent() {}

    virtual void DoWork() {
        if (!Thread::MyThread()->IsShuttingDown())
        {
            const int timeoutInSeconds(1);
            const bool throwOnShutdown(false);
            Thread::InterruptibleSleep(timeoutInSeconds, throwOnShutdown);
            mDispatcher->Enqueue(make_shared<ReenqueueingEvent>(mDispatcher));
        }
    }

protected:
    DispatcherPtr mDispatcher;
};

class TestRequestHandler : public RequestHandler
{
public:
    TestRequestHandler() : RequestHandler(0), mHandledRequests(0) {}
    virtual ~TestRequestHandler() {}

    void Handler(Event *e) {
        FTRACE;
        mHandledRequests++;

        TestEvent* event = dynamic_cast<TestEvent*>(e);
        if (event != NULL)
        {
            event->DoWork();
        }
        else
        {
            cout << "Got unexpected event" << endl;
        }
    }

    void Busy(void) {
        FTRACE;
    }

    void Periodic(void) {
        FTRACE;
    }

    void Init(void) {
        FTRACE;
    }

    void Cleanup(void) {
        FTRACE;
    }

    int mHandledRequests;
};


class ThreadPoolDispatcherUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
        hlog(HLOG_DEBUG, "Starting test...");
    }

    static void TearDownTestCase() {
    }

    void SetUp() {
    }

    void TearDown() {
    }
};

TEST_F(ThreadPoolDispatcherUnitTest, ConstructDelete)
{
    FTRACE;

    boost::shared_ptr<TestRequestHandler> testHandler(new TestRequestHandler());
    boost::shared_ptr<RequestHandler> handler = testHandler;

    int minThreads = 6;
    int maxThreads = 6;
    int minSpareThreads = 1;
    int maxSpareThreads = 1;
    int deepQueue = 32;
    int maxDepth = 32;

    boost::shared_ptr<ThreadPoolDispatcher> dispatcher(
        new ThreadPoolDispatcher(
            handler,
            minThreads,
            maxThreads,
            minSpareThreads,
            maxSpareThreads,
            deepQueue,
            maxDepth,
            "TestThreadPool"
            )
        );
}


TEST_F(ThreadPoolDispatcherUnitTest, ConstructDeleteWithEnqueuedEvent)
{
    FTRACE;

    boost::shared_ptr<TestRequestHandler> testHandler(new TestRequestHandler());
    boost::shared_ptr<RequestHandler> handler = testHandler;

    int minThreads = 6;
    int maxThreads = 6;
    int minSpareThreads = 1;
    int maxSpareThreads = 1;
    int deepQueue = 32;
    int maxDepth = 32;

    boost::shared_ptr<ThreadPoolDispatcher> dispatcher(
        new ThreadPoolDispatcher(
            handler,
            minThreads,
            maxThreads,
            minSpareThreads,
            maxSpareThreads,
            deepQueue,
            maxDepth,
            "TestThreadPool"
            )
        );

    dispatcher->Enqueue(make_shared<TestEventDoOutput>());

    //TODO: wait more intelligently for these events to finish
    sleep(1);

    ASSERT_EQ(1, testHandler->mHandledRequests);
}

TEST_F(ThreadPoolDispatcherUnitTest, WorkersCanWorkUntilShutdown)
{
    FTRACE;

    boost::shared_ptr<TestRequestHandler> testHandler(new TestRequestHandler());
    boost::shared_ptr<RequestHandler> handler = testHandler;

    int minThreads = 6;
    int maxThreads = 6;
    int minSpareThreads = 1;
    int maxSpareThreads = 1;
    int deepQueue = 32;
    int maxDepth = 32;

    boost::shared_ptr<ThreadPoolDispatcher> dispatcher(
        new ThreadPoolDispatcher(
            handler,
            minThreads,
            maxThreads,
            minSpareThreads,
            maxSpareThreads,
            deepQueue,
            maxDepth,
            "TestThreadPool"
            )
        );

    dispatcher->Enqueue(make_shared<TestEventWorkUntilSignaled>());
    dispatcher->Enqueue(make_shared<TestEventWorkUntilSignaled>());
    dispatcher->Enqueue(make_shared<TestEventWorkUntilSignaled>());
    dispatcher->Enqueue(make_shared<TestEventWorkUntilSignaled>());
    dispatcher->Enqueue(make_shared<TestEventWorkUntilSignaled>());

    sleep(5);
    const int maxEvents(10);
    std::list<shared_ptr<Event> > events;

    ASSERT_EQ(0, dispatcher->GetQueuedEvents(maxEvents, events));
    ASSERT_EQ(5, dispatcher->GetRunningEvents(maxEvents, events));
    ASSERT_EQ(5, testHandler->mHandledRequests);

    dispatcher->Shutdown();

    ASSERT_EQ(5, testHandler->mHandledRequests);
}

TEST_F(ThreadPoolDispatcherUnitTest, WorkersCanReenqueueSelves)
{
    FTRACE;

    boost::shared_ptr<TestRequestHandler> testHandler(new TestRequestHandler());
    boost::shared_ptr<RequestHandler> handler = testHandler;

    int minThreads = 6;
    int maxThreads = 6;
    int minSpareThreads = 1;
    int maxSpareThreads = 1;
    int deepQueue = 32;
    int maxDepth = 32;

    boost::shared_ptr<ThreadPoolDispatcher> dispatcher(
        new ThreadPoolDispatcher(
            handler,
            minThreads,
            maxThreads,
            minSpareThreads,
            maxSpareThreads,
            deepQueue,
            maxDepth,
            "TestThreadPool"
            )
        );

    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));

    sleep(5);
    //const int maxEvents(10);
    //std::list<shared_ptr<Event> > events;

    dispatcher->Shutdown();
}

TEST_F(ThreadPoolDispatcherUnitTest, MultipleDispatchersCanExistInSameProcess)
{
    FTRACE;

    boost::shared_ptr<TestRequestHandler> testHandler(new TestRequestHandler());
    boost::shared_ptr<RequestHandler> handler = testHandler;

    int minThreads = 6;
    int maxThreads = 6;
    int minSpareThreads = 1;
    int maxSpareThreads = 1;
    int deepQueue = 32;
    int maxDepth = 32;

    boost::shared_ptr<ThreadPoolDispatcher> dispatcher(
        new ThreadPoolDispatcher(
            handler,
            minThreads,
            maxThreads,
            minSpareThreads,
            maxSpareThreads,
            deepQueue,
            maxDepth,
            "TestThreadPool"
            )
        );

    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));

    // 2
    boost::shared_ptr<TestRequestHandler> testHandler2(new TestRequestHandler());
    boost::shared_ptr<RequestHandler> handler2 = testHandler;
    boost::shared_ptr<ThreadPoolDispatcher> dispatcher2(
        new ThreadPoolDispatcher(
            handler2,
            minThreads,
            maxThreads,
            minSpareThreads,
            maxSpareThreads,
            deepQueue,
            maxDepth,
            "TestThreadPool2"
            )
        );
    dispatcher2->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher2->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher2->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher2->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher2->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));

    // 3
    boost::shared_ptr<TestRequestHandler> testHandler3(new TestRequestHandler());
    boost::shared_ptr<RequestHandler> handler3 = testHandler;
    boost::shared_ptr<ThreadPoolDispatcher> dispatcher3(
        new ThreadPoolDispatcher(
            handler3,
            minThreads,
            maxThreads,
            minSpareThreads,
            maxSpareThreads,
            deepQueue,
            maxDepth,
            "TestThreadPool3"
            )
        );
    dispatcher3->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher3->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher3->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher3->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));
    dispatcher3->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));


    sleep(5);
    //const int maxEvents(10);
    //std::list<shared_ptr<Event> > events;

    dispatcher->Shutdown();
    dispatcher2->Shutdown();
    dispatcher3->Shutdown();
}

TEST_F(ThreadPoolDispatcherUnitTest, CanCreateThreadPoolWithExactNumberOfThreads)
{
    FTRACE;

    boost::shared_ptr<TestRequestHandler> testHandler(new TestRequestHandler());
    boost::shared_ptr<RequestHandler> handler = testHandler;

    int minThreads = 6;
    int maxThreads = 6;
    int minSpareThreads = 6;
    int maxSpareThreads = 6;
    int deepQueue = 32;
    int maxDepth = 32;

    boost::shared_ptr<ThreadPoolDispatcher> dispatcher(
        new ThreadPoolDispatcher(
            handler,
            minThreads,
            maxThreads,
            minSpareThreads,
            maxSpareThreads,
            deepQueue,
            maxDepth,
            "SixThreadPool"
            )
        );

    dispatcher->Enqueue(make_shared<TestEventDoOutput>());
    dispatcher->Enqueue(make_shared<TestEventWorkUntilSignaled>());
    dispatcher->Enqueue(make_shared<ReenqueueingEvent>(dispatcher));

    for (int i = 0; i<10; i++)
    {
        sleep(1);
        EXPECT_EQ(6, dispatcher->GetThreadCount());
    }
}
