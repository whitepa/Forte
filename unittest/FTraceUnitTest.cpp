#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"

#include "FTrace.h"

using Forte::LogManager;
using Forte::FTrace;
using ::testing::UnitTest;

LogManager logManager;

class FTraceUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging(
            "//stderr",
            //logManager.GetSingleLevelFromString("UPTO_DEBUG"),
            HLOG_ALL, // since this is an FTRACE test
            HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
    }

    static void TearDownTestCase() {
        logManager.EndLogging();
    }

    void SetUp() {
        hlogstream(
            HLOG_INFO, "Starting test "
            << UnitTest::GetInstance()->current_test_info()->name()
            << "."
            << UnitTest::GetInstance()->current_test_info()->test_case_name());
    }

    void TearDown() {
        hlogstream(
            HLOG_INFO, "ending test "
            << UnitTest::GetInstance()->current_test_info()->name()
            << "."
            << UnitTest::GetInstance()->current_test_info()->test_case_name());
    }
};

TEST_F(FTraceUnitTest, ConstructDelete)
{
    FTRACE;
}

TEST_F(FTraceUnitTest, LogsUpToMaxBufferSize)
{
    std::string s(65000, 'a');
    FTRACESTREAM(s);
}
