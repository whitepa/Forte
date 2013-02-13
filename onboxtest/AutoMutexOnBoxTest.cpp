#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "AutoMutex.h"
#include <boost/make_shared.hpp>
#include "FTrace.h"
#include <boost/thread/mutex.hpp>

// #SCQAD TEST: ONBOX: AutoMutexOnBoxTest

// #SCQAD TESTTAG: smoketest, forte

using namespace Forte;
using ::testing::_;
using ::testing::StrEq;
using ::testing::Throw;
using ::testing::Return;
using ::testing::StrictMock;

LogManager logManager;

// This is taken from scqad/LeaderActions.h. I think it would be nice
// to have in forte. Clock.h seems a natural spot, but that header is
// getting kind of big. Just copying here for now
class SimpleTimer
{
public:
    SimpleTimer() {}
    virtual ~SimpleTimer() {}

    void Start() {
        mStartTime = mMonoTime.GetTime();
        mStartTimeReal = mRealTime.GetTime();
    }
    void Stop() {
        // @TODO: check if started
        mEndTime = mMonoTime.GetTime();
        mEndTimeReal = mRealTime.GetTime();
    }
    unsigned int GetRunTimeInSeconds() {
        // @TODO: check if started and stopped
        return (mEndTime.AsSeconds() - mStartTime.AsSeconds());
    }

    long long GetRunTimeRealInSeconds() {
        // @TODO: check if started and stopped
        return (mEndTimeReal.AsMillisec() - mStartTimeReal.AsMillisec());
    }

    Forte::MonotonicClock mMonoTime;
    Forte::RealtimeClock mRealTime;
    Forte::Timespec mStartTime;
    Forte::Timespec mEndTime;
    Forte::Timespec mStartTimeReal;
    Forte::Timespec mEndTimeReal;
};

class AutoMutexOnBoxTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        //logManager.BeginLogging("//stdout");
        logManager.BeginLogging(__FILE__ ".log");
    }

    void SetUp() {
    }

    Mutex mForteMutex;
    boost::mutex mBoostMutex;
};

// this seems like too small of a sample size
TEST_F(AutoMutexOnBoxTest, DISABLED_IsEquivalentToBoostLockGuardAt1000000Locks)
{
    FTRACE;

    SimpleTimer forteTimer;
    forteTimer.Start();
    for (int i = 0; i < 1000000; i++)
    {
        AutoUnlockMutex guard(mForteMutex);
    }
    forteTimer.Stop();


    SimpleTimer boostTimer;
    boostTimer.Start();
    for (int i = 0; i < 1000000; i++)
    {
        boost::lock_guard<boost::mutex> guard(mBoostMutex);
    }
    boostTimer.Stop();

    hlogstream(HLOG_INFO,
               "forte time: " << forteTimer.GetRunTimeRealInSeconds()
               << " boost time: " << boostTimer.GetRunTimeRealInSeconds());

    // With this small of sample size, giving a lot of leeway to
    // screen out false failures
    ASSERT_GT(boostTimer.GetRunTimeRealInSeconds() + 10,
              forteTimer.GetRunTimeRealInSeconds());
}

TEST_F(AutoMutexOnBoxTest, IsEquivalentToBoostLockGuardAt100000000Locks)
{
    FTRACE;

    SimpleTimer boostTimer;
    boostTimer.Start();
    for (int i = 0; i < 100000000; i++)
    {
        boost::lock_guard<boost::mutex> guard(mBoostMutex);
    }
    boostTimer.Stop();

    SimpleTimer forteTimer;
    forteTimer.Start();
    for (int i = 0; i < 100000000; i++)
    {
        AutoUnlockMutex guard(mForteMutex);
    }
    forteTimer.Stop();

    hlogstream(HLOG_INFO,
               "forte time: " << forteTimer.GetRunTimeRealInSeconds()
               << " boost time: " << boostTimer.GetRunTimeRealInSeconds());

    // these have been seen to take around 3000 seconds, giving 10%
    // handicap to avoid false failures
    ASSERT_GT(boostTimer.GetRunTimeRealInSeconds() + 300,
              forteTimer.GetRunTimeRealInSeconds());
}

TEST_F(AutoMutexOnBoxTest, IsEquivalentToBoostLockGuardAt1000000000Locks)
{
    FTRACE;

    SimpleTimer boostTimer;
    boostTimer.Start();
    for (int i = 0; i < 1000000000; i++)
    {
        boost::lock_guard<boost::mutex> guard(mBoostMutex);
    }
    boostTimer.Stop();

    SimpleTimer forteTimer;
    forteTimer.Start();
    for (int i = 0; i < 1000000000; i++)
    {
        AutoUnlockMutex guard(mForteMutex);
    }
    forteTimer.Stop();

    hlogstream(HLOG_INFO,
               "forte time: " << forteTimer.GetRunTimeRealInSeconds()
               << " boost time: " << boostTimer.GetRunTimeRealInSeconds());

    // these have been seen to take around 30000 seconds, giving 10%
    // handicap to avoid false failures
    ASSERT_GT(boostTimer.GetRunTimeRealInSeconds() + 3000,
              forteTimer.GetRunTimeRealInSeconds());
}
