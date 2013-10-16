#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "ProcessManagerImpl.h"
#include "ProcessFuture.h"
#include "Future.h"
#include "AutoFD.h"
#include "FileSystem.h"
#include "ProcFileSystem.h"
#include "AutoDoUndo.h"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

// #SCQAD TESTTAG: smoketest, forte, forte.procmon

using namespace Forte;
using ::testing::UnitTest;

LogManager logManager;

#pragma GCC diagnostic ignored "-Wold-style-cast"

struct TestParams
{
    TestParams(int p, int s)
        : numProcesses(p),
          numSeconds(s)
        {}

    int numProcesses;
    int numSeconds;
};

class ProcessManagerTest : public ::testing::TestWithParam<TestParams>
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
            << UnitTest::GetInstance()->current_test_info()->test_case_name());
    }

    void TearDown() {
        hlogstream(
            HLOG_INFO, "ending test "
            << UnitTest::GetInstance()->current_test_info()->test_case_name());
    }
};

#pragma GCC diagnostic warning "-Wold-style-cast"

TEST_P(ProcessManagerTest, RunsXProcessesInUnderYSeconds)
{
    TestParams p(GetParam());

    try
    {
        boost::shared_ptr<ProcessManager> pm(new ProcessManagerImpl);
        std::list<boost::shared_ptr<ProcessFuture> > phList;
        Forte::Timespec start = Forte::MonotonicClock().GetTime();
        for (int i = 0; i < p.numProcesses; ++i)
        {
            phList.push_back(
                boost::shared_ptr<ProcessFuture>(
                    pm->CreateProcess("/bin/sleep 1")));
        }
        foreach(const boost::shared_ptr<Forte::ProcessFuture> &ph, phList)
        {
            ASSERT_NO_THROW(ph->GetResult());
            ASSERT_EQ(0, ph->GetStatusCode());
            ASSERT_EQ(ProcessFuture::ProcessExited,
                      ph->GetProcessTerminationType());
        }
        Forte::Timespec finish = Forte::MonotonicClock().GetTime();
        ASSERT_LT((finish - start), Forte::Timespec::FromSeconds(p.numSeconds));
    }
    catch (std::exception& e)
    {
        hlog(HLOG_ERR, "exception: %s", e.what());
        FAIL();
    }
}

INSTANTIATE_TEST_CASE_P(TimedRunProcesses,
                        ProcessManagerTest,
                        ::testing::Values(
                            TestParams(1,10),
                            TestParams(2,10),
                            TestParams(3,10),
                            TestParams(10,10),
                            TestParams(50,10),
                            TestParams(100,10)));

TEST_F(ProcessManagerTest, ReactsToChildTerminationInUnder100Millisec)
{
    try
    {
        boost::shared_ptr<ProcessManager> pm(new ProcessManagerImpl);
        boost::shared_ptr<ProcessFuture> ph(
            pm->CreateProcess("/bin/usleep 10"));
        Forte::Timespec start = Forte::MonotonicClock().GetTime();
        ASSERT_NO_THROW(ph->GetResult());
        Forte::Timespec finish = Forte::MonotonicClock().GetTime();
        ASSERT_EQ(0, ph->GetStatusCode());
        ASSERT_EQ(ProcessFuture::ProcessExited, ph->GetProcessTerminationType());
        ASSERT_LT(finish - start, Forte::Timespec::FromMillisec(100));
    }
    catch (std::exception& e)
    {
        hlog(HLOG_ERR, "exception: %s", e.what());
        FAIL();
    }
}
