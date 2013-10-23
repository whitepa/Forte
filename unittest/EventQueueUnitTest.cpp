#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "FTrace.h"
#include "LogManager.h"

#include "Event.h"
#include "EventQueue.h"

using namespace std;
using namespace boost;
using namespace Forte;

using ::testing::_;
using ::testing::Return;
using ::testing::UnitTest;

LogManager logManager;

class EventQueueUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
    }

    static void TearDownTestCase() {
        logManager.EndLogging();
    }

    void SetUp() {
        hlogstream(
            HLOG_INFO, "Starting test "
            << UnitTest::GetInstance()->current_test_info()->name());
    }

    void TearDown() {
        hlogstream(
            HLOG_INFO, "ending test "
            << UnitTest::GetInstance()->current_test_info()->name());
    }
};

TEST_F(EventQueueUnitTest, ConstructDelete)
{
    EventQueue theClass;
}

TEST_F(EventQueueUnitTest, ConstructDeleteDropOldest)
{
    EventQueue theClass(EventQueue::QUEUE_MODE_DROP_OLDEST_LOG);
}

TEST_F(EventQueueUnitTest, DropOldestStartsDroppingWhenFull)
{
    const int maxDepth(4);
    EventQueue q(maxDepth, EventQueue::QUEUE_MODE_DROP_OLDEST_LOG);
    boost::shared_ptr<Event> e(new Event());
    q.Add(e);
    EXPECT_EQ(1, q.Depth());

    q.Add(e);
    q.Add(e);
    q.Add(e);

    EXPECT_EQ(4, q.Depth());

    q.Add(e);

    EXPECT_EQ(4, q.Depth());
}

TEST_F(EventQueueUnitTest, DropOldestStopsDroppingAfterSpaceIsAvailable)
{
    const int maxDepth(4);
    EventQueue q(maxDepth, EventQueue::QUEUE_MODE_DROP_OLDEST_LOG);
    boost::shared_ptr<Event> e(new Event());

    for (int i=0; i<100; ++i)
    {
        q.Add(e);
        q.Get();
        EXPECT_EQ(0, q.Depth());
    }

    q.Add(e);
    q.Get();
    EXPECT_EQ(0, q.Depth());

    q.Add(e);
    q.Get();
    EXPECT_EQ(0, q.Depth());

    q.Add(e);
    EXPECT_EQ(1, q.Depth());
    q.Add(e);
    EXPECT_EQ(2, q.Depth());
    q.Add(e);
    EXPECT_EQ(3, q.Depth());
    q.Add(e);
    EXPECT_EQ(4, q.Depth());
}
