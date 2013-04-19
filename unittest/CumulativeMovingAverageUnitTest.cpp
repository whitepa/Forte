#include <gtest/gtest.h>
#include "CumulativeMovingAverage.h"
#include "LogManager.h"

using namespace Forte;

LogManager logManager;

class CumulativeMovingAverageUnitTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.BeginLogging("//stdout");
        logManager.SetLogMask("//stdout", HLOG_ALL);
        hlog(HLOG_DEBUG, "Starting test...");
    }

    static void TearDownTestCase() {
        logManager.EndLogging("//stdout");
    }

};

TEST_F(CumulativeMovingAverageUnitTest, NoDataPoints)
{
    CumulativeMovingAverage average;

    ASSERT_EQ(0, static_cast<int64_t>(average));
}

TEST_F(CumulativeMovingAverageUnitTest, OneDataPoints)
{
    CumulativeMovingAverage average;

    average = 10;

    ASSERT_EQ(10, static_cast<int64_t>(average));
}

TEST_F(CumulativeMovingAverageUnitTest, TwoDataPoints)
{
    CumulativeMovingAverage average;

    average = 10;
    average = 20;

    ASSERT_EQ(((10 + 20) / 2), static_cast<int64_t>(average));
}

TEST_F(CumulativeMovingAverageUnitTest, ThreeDataPoints)
{
    CumulativeMovingAverage average;

    average = 10;
    average = 20;
    average = 32;

    ASSERT_EQ(((10 + 20 + 32) / 3), static_cast<int64_t>(average));
}

TEST_F(CumulativeMovingAverageUnitTest, FourDataPoints)
{
    CumulativeMovingAverage average;

    average = 10;
    average = 20;
    average = 32;
    average = 33;

    ASSERT_EQ(((10 + 20 + 32 + 33) / 4), static_cast<int64_t>(average));
}
