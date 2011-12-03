#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "Util.h"
#include "Clock.h"

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

