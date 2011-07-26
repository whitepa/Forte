#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "RunLoop.h"
#include "Timer.h"
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

using namespace Forte;
using namespace boost;

LogManager logManager;

class RunLoopTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");
    }
};

class TimerTarget : public Forte::Object
{
public:
    TimerTarget(void) : mCount(0) {};

    void TimerFired(void) { hlog(HLOG_INFO, "timer fired!"); mCount++; }
    int mCount;
};

TEST_F(RunLoopTest, Basic)
{
    shared_ptr<TimerTarget> target = make_shared<TimerTarget>();
    shared_ptr<RunLoop> rl = make_shared<RunLoop>();
    shared_ptr<Timer> t = make_shared<Timer>(rl, target, boost::bind(&TimerTarget::TimerFired, target),
                                             Forte::Timespec::FromSeconds(1), false);
    rl->AddTimer(t);
    ASSERT_TRUE(target->mCount == 0);
    usleep(1100000);
    ASSERT_TRUE(target->mCount == 1);
}

TEST_F(RunLoopTest, Multi)
{
    shared_ptr<TimerTarget> target = make_shared<TimerTarget>();
    shared_ptr<RunLoop> rl = make_shared<RunLoop>();
    std::list<shared_ptr<Timer> > timers;
    for (int i = 1; i < 11; ++i)
    {
        shared_ptr<Timer> t = make_shared<Timer>(rl, target, boost::bind(&TimerTarget::TimerFired, target),
                                                 Forte::Timespec::FromMillisec(100*i), false);
        rl->AddTimer(t);
        timers.push_back(t);
    }

    ASSERT_TRUE(target->mCount == 0);
    usleep(150000);
    for (int i = 1; i < 11; ++i)
    {
        ASSERT_EQ(target->mCount, i);
        usleep(100000);
    }
}

TEST_F(RunLoopTest, ScheduledInPast)
{
    shared_ptr<TimerTarget> target = make_shared<TimerTarget>();
    shared_ptr<RunLoop> rl = make_shared<RunLoop>();
    std::list<shared_ptr<Timer> > timers;

    for (int i = 0; i < 5; ++i)
    {
        shared_ptr<Timer> t = make_shared<Timer>(rl, target, 
                                                 boost::bind(&TimerTarget::TimerFired, target),
                                                 Forte::Timespec::FromMillisec(-100*i), false);
        rl->AddTimer(t);
        timers.push_back(t);
    }
    usleep(1000);
    ASSERT_EQ(target->mCount, 5);
}
