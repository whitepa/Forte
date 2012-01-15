#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "Util.h"
#include "Clock.h"
#include <algorithm>

using namespace Forte;
using namespace boost;

LogManager logManager;

class ClockUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");
    }
};

;

TEST_F(ClockUnitTest, TimespecOperations)
{
    Timespec a(10, 2);
    Timespec b(-3 , 3); // -3 + 3 nanosecods = 2.999999997

    ASSERT_FALSE((a - a).IsPositive());
    ASSERT_TRUE((a - a).IsZero());
    ASSERT_TRUE(a == a);
    ASSERT_TRUE((b + a) == (a + b));

    // -2.999999997 - 10.000000002
    ASSERT_TRUE((b - a) == Timespec(-13, 1));
    ASSERT_TRUE((b - a - Timespec(0, 1)) == Timespec(-13, 0));
}

TEST_F(ClockUnitTest, TimespecCtorThrowsWhenInvalidNanoseconds)
{
    ASSERT_THROW (const Timespec b(1, 1000000000), ETimespecInvalid);
}

TEST_F(ClockUnitTest, TimespecStdMinOperatorReturnRhsSecondsOnly)
{
    const Timespec a(1, 0);
    const Timespec b(0, 0);

    ASSERT_EQ (b, std::min(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMinOperatorReturnLhsSecondsOnly)
{
    const Timespec a(0, 0);
    const Timespec b(1, 0);

    ASSERT_EQ (a, std::min(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMinOperatorReturnRhsNanoSecondsOnly)
{
    const Timespec a(0, 1);
    const Timespec b(0, 0);

    ASSERT_EQ (b, std::min(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMinOperatorReturnLhsNanoSecondsOnly)
{
    const Timespec a(0, 0);
    const Timespec b(0, 1);

    ASSERT_EQ (a, std::min(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMinOperatorReturnRhs)
{
    const Timespec a(2, 0);
    const Timespec b(1, 999999999);

    ASSERT_EQ (b, std::min(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMinOperatorReturnLhs)
{
    const Timespec a(1, 999999999);
    const Timespec b(2, 0);

    ASSERT_EQ (a, std::min(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMaxOperatorReturnRhsSecondsOnly)
{
    const Timespec a(0, 0);
    const Timespec b(1, 0);

    ASSERT_EQ (b, std::max(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMaxOperatorReturnLhsSecondsOnly)
{
    const Timespec a(1, 0);
    const Timespec b(0, 0);

    ASSERT_EQ (a, std::max(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMaxOperatorReturnRhsNanoSecondsOnly)
{
    const Timespec a(0, 0);
    const Timespec b(0, 1);

    ASSERT_EQ (b, std::max(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMaxOperatorReturnLhsNanoSecondsOnly)
{
    const Timespec a(0, 1);
    const Timespec b(0, 0);

    ASSERT_EQ (a, std::max(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMaxOperatorReturnRhs)
{
    const Timespec a(1, 999999999);
    const Timespec b(2, 0);

    ASSERT_EQ (b, std::max(a, b));
}

TEST_F(ClockUnitTest, TimespecStdMaxOperatorReturnLhs)
{
    const Timespec a(2, 0);
    const Timespec b(1, 999999999);

    ASSERT_EQ (a, std::max(a, b));
}

TEST_F(ClockUnitTest, DeadlineClockExpiresAfterXSeconds)
{
    DeadlineClock expiresInOneSecond;
    expiresInOneSecond.ExpiresInSeconds(1);

    EXPECT_FALSE(expiresInOneSecond.Expired());
    sleep(1);
    usleep(100);
    EXPECT_TRUE(expiresInOneSecond.Expired());
}

TEST_F(ClockUnitTest, DeadlineClockExpiresAfterXMillisec)
{
    DeadlineClock expiresIn500Millisec;
    expiresIn500Millisec.ExpiresInMillisec(500);

    EXPECT_FALSE(expiresIn500Millisec.Expired());
    //usleep(500001);
    usleep(600000); //Testing Deadline functionality, not hw functionality
    EXPECT_TRUE(expiresIn500Millisec.Expired());
}
