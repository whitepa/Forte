#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"

#include "OnDemandDispatcher.h"

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

class TestRequestHandler : public RequestHandler
{
public:
    TestRequestHandler()
        : RequestHandler(0),
          mHandledRequestsCondition(mHandledRequestsMutex),
          mHandledRequests(0)
        {
        }
    virtual ~TestRequestHandler() {}

    void Handler(Event *e) {
        FTRACE;

        TestEvent* event = dynamic_cast<TestEvent*>(e);
        if (event != NULL)
        {
            event->DoWork();
        }
        else
        {
            cout << "Got unexpected event" << endl;
        }

        AutoUnlockMutex lock(mHandledRequestsMutex);
        mHandledRequests++;
        mHandledRequestsCondition.Broadcast();
        mHandledRequestsCondition.Signal();
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

    int GetHandledRequestCount() {
        AutoUnlockMutex lock(mHandledRequestsMutex);
        return mHandledRequests;
    }

    void WaitForRequest(int minCount = 1) {
        AutoUnlockMutex lock(mHandledRequestsMutex);
        while (mHandledRequests < minCount)
        {
            mHandledRequestsCondition.Wait();
        }
    }

protected:
    Forte::Mutex mHandledRequestsMutex;
    Forte::ThreadCondition mHandledRequestsCondition;
    int mHandledRequests;
};

class OnDemandDispatcherUnitTest : public ::testing::Test
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
        logManager.EndLogging();
    }

    void SetUp() {
    }

    void TearDown() {
    }
};

TEST_F(OnDemandDispatcherUnitTest, ConstructDelete)
{
    FTRACE;

    boost::shared_ptr<TestRequestHandler> testHandler(new TestRequestHandler());
    boost::shared_ptr<RequestHandler> handler = testHandler;

    const int maxThreads = 1;
    int deepQueue = 32;
    int maxDepth = 32;
    const char* name = "TestOnDemandDispatcher";

    OnDemandDispatcher dispatcher(
        testHandler,
        maxThreads,
        deepQueue,
        maxDepth,
        name);

    dispatcher.Shutdown();
}

TEST_F(OnDemandDispatcherUnitTest , CreateAThreadPerEvent)
{
    FTRACE;

    boost::shared_ptr<TestRequestHandler> testHandler(new TestRequestHandler());
    boost::shared_ptr<RequestHandler> handler = testHandler;

    const int maxThreads = 1;
    int deepQueue = 32;
    int maxDepth = 32;
    const char* name = "TestOnDemandDispatcher";

    OnDemandDispatcher dispatcher(
        testHandler,
        maxThreads,
        deepQueue,
        maxDepth,
        name);

    for (int i=0; i<maxThreads; i++)
    {
        dispatcher.Enqueue(make_shared<TestEventDoOutput>());
    }

    testHandler->WaitForRequest(1);
    ASSERT_EQ(maxThreads, testHandler->GetHandledRequestCount());

    dispatcher.Shutdown();
}

