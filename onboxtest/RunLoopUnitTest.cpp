#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "RunLoop.h"
#include "Timer.h"
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

// #SCQAD TEST: ONBOX: RunLoopUnitTest

// #SCQAD TESTTAG: smoketest, forte

using namespace Forte;
using namespace boost;

LogManager logManager;

class RunLoopTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stdout", HLOG_INFO, HLOG_FORMAT_SIMPLE);
    }

    static void TearDownTestCase() {
        logManager.EndLogging();
    }
};

class TimerTarget : public Forte::Object
{
public:
    TimerTarget(void) : mCount(0) {};

    void TimerFired(void) {
        //AutoUnlockMutex mLock(mMutex);
        mCount++;
        hlog(HLOG_INFO, "timer fired! (mCount now: %i)", mCount);
    }
    int mCount;
    Mutex mMutex;
};

TEST_F(RunLoopTest, Basic)
{
    boost::shared_ptr<TimerTarget> target = make_shared<TimerTarget>();
    boost::shared_ptr<RunLoop> rl = make_shared<RunLoop>("test");
    boost::shared_ptr<Timer> t = make_shared<Timer>("TimerTarget::TimerFired",
        rl, boost::bind(&TimerTarget::TimerFired, target),
        Forte::Timespec::FromSeconds(1), false);
    rl->AddTimer(t);
    EXPECT_EQ(0, target->mCount);
    usleep(1100000);
    EXPECT_EQ(1, target->mCount);
    rl->Shutdown();
    rl->WaitForShutdown();
}

// NOTE: this test has been backed down because the run time
// environment does not seem to be reliable enough to always hit the
// microsecond precisions.
TEST_F(RunLoopTest, Multi)
{
    boost::shared_ptr<TimerTarget> target = make_shared<TimerTarget>();
    boost::shared_ptr<RunLoop> rl = make_shared<RunLoop>("test");
    std::list<boost::shared_ptr<Timer> > timers;
    for (int i = 1; i < 11; ++i)
    {
        boost::shared_ptr<Timer> t = make_shared<Timer>("TimerTarget::TimerFired",
            rl, boost::bind(&TimerTarget::TimerFired, target),
            Forte::Timespec::FromMillisec(1000*i), false);
        rl->AddTimer(t);
        timers.push_back(t);
    }

    while (target->mCount == 0)
    {
        usleep(1000);
    }

    usleep(1000);

    for (int i = 1; i < 11; ++i)
    {
        EXPECT_EQ(i, target->mCount);
        usleep(1000000);
    }
    rl->Shutdown();
    rl->WaitForShutdown();
}

TEST_F(RunLoopTest, ScheduledInPast)
{
    boost::shared_ptr<TimerTarget> target = make_shared<TimerTarget>();
    boost::shared_ptr<RunLoop> rl = make_shared<RunLoop>("test");
    std::list<boost::shared_ptr<Timer> > timers;

    for (int i = 0; i < 5; ++i)
    {
        boost::shared_ptr<Timer> t = make_shared<Timer>("TimerTarget::TimerFired",
            rl,
            boost::bind(&TimerTarget::TimerFired, target),
            Forte::Timespec::FromMillisec(-100*i), false);
        rl->AddTimer(t);
        timers.push_back(t);
    }
    usleep(10000);
    EXPECT_EQ(5, target->mCount);
    rl->Shutdown();
    rl->WaitForShutdown();
}
