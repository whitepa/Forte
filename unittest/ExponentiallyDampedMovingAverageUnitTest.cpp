#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "FTrace.h"
#include "ContextImpl.h"
#include "LogManager.h"

#include "ExponentiallyDampedMovingAverage.h"

using namespace std;
using namespace boost;
using namespace Forte;

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::AtLeast;
using ::testing::SetArgReferee;

LogManager logManager;

class ExponentiallyDampedMovingAverageUnitTest : public ::testing::Test {
public:
    virtual void SetUp() {
        mValue = 0;
        mRate = 0;
    }
    static void SetUpTestCase() {
        logManager.SetGlobalLogMask(HLOG_NODEBUG);
        logManager.BeginLogging("//stderr");
    }
    static void TearDownTestCase() {
        logManager.EndLogging();
    }
    int64_t GetValue(void) {
        Forte::AutoUnlockMutex lock(mLock);
        return mValue;
    }
    void SetValue(int64_t value) {
        Forte::AutoUnlockMutex lock(mLock);
        mValue = value;
    }

    int64_t GetRate(void) {
        Forte::AutoUnlockMutex lock(mLock);
        int64_t val = mRate;
        mRate = 0;
        return val;
    }
    void IncrForRate(int64_t value) {
        Forte::AutoUnlockMutex lock(mLock);
        mRate += value;
    }
    ContextImpl mContext;
    Forte::Mutex mLock;
    int64_t mValue;
    int64_t mRate;
};

TEST_F(ExponentiallyDampedMovingAverageUnitTest, Value)
{
    logManager.SetGlobalLogMask(HLOG_ALL);
    shared_ptr<Forte::RunLoop> rl(new RunLoop("stats"));
    ExponentiallyDampedMovingAverage edma(11,
                                          Forte::Timespec::FromMillisec(200),
                                          Forte::Timespec::FromMillisec(50),
                                          boost::bind(
                                              &ExponentiallyDampedMovingAverageUnitTest::GetValue,
                                              this),
                                          rl);
    SetValue(0);
    usleep(100000);
    EXPECT_EQ(0.0, static_cast<double>(edma));
    SetValue(5);
    sleep(2);
    EXPECT_GT(static_cast<double>(edma), 4.99);
    EXPECT_LT(static_cast<double>(edma), 5.01);
    SetValue(10);
    EXPECT_GT(static_cast<double>(edma), 4.99);
    EXPECT_LT(static_cast<double>(edma), 5.01);
    sleep(1);
    EXPECT_GT(static_cast<double>(edma), 9.0);
    EXPECT_LT(static_cast<double>(edma), 11.0);
    logManager.SetGlobalLogMask(HLOG_NODEBUG);
    rl->Shutdown();
    rl->WaitForShutdown();
}

TEST_F(ExponentiallyDampedMovingAverageUnitTest, Rate)
{
    logManager.SetGlobalLogMask(HLOG_ALL);
    shared_ptr<Forte::RunLoop> rl(new RunLoop("stats"));
    ExponentiallyDampedMovingAverage rate(11,
                                          Forte::Timespec::FromMillisec(200),
                                          Forte::Timespec::FromMillisec(100),
                                          boost::bind(
                                              &ExponentiallyDampedMovingAverageUnitTest::GetRate,
                                              this),
                                          rl);
    Forte::MonotonicClock mc;
    Forte::Timespec start, finish;
    start = mc.GetTime();
    for (int i = 0; i < 100; ++i)
    {
        IncrForRate(1);
        usleep(10000);
        // 100 per second
    }
    finish = mc.GetTime();
    Forte::Timespec elapsed = finish - start;
    double expectedRate = 100.0 / (static_cast<double>(elapsed.AsMillisec()) / 1000.0);
    double tolerance = 10.0;
    Forte::Timespec persecond = Forte::Timespec::FromSeconds(1);
    hlogstream(HLOG_INFO, "rate " << rate.AsRate(persecond) << " expected " << expectedRate << " (" << elapsed.AsMillisec() << "ms elapsed)");
    EXPECT_GT(rate.AsRate(persecond), expectedRate - tolerance);
    EXPECT_LT(rate.AsRate(persecond), expectedRate + tolerance);
    logManager.SetGlobalLogMask(HLOG_NODEBUG);
    rl->Shutdown();
    rl->WaitForShutdown();
}
