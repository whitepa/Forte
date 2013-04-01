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

// #SCQAD TEST: ONBOX: ProcessManagerOnBoxTest

// #SCQAD TESTTAG: smoketest, forte, forte.procmon

using namespace Forte;

LogManager logManager;

class ProcessManagerTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        signal(SIGPIPE, SIG_IGN);
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");
    };
    static void TearDownTestCase() {

    };

};

TEST_F(ProcessManagerTest, Runs100ProcessesInUnder10Seconds)
{
    try
    {
        boost::shared_ptr<ProcessManager> pm(new ProcessManagerImpl);
        std::list<boost::shared_ptr<ProcessFuture> > phList;
        Forte::Timespec start = Forte::MonotonicClock().GetTime();
        for (int i = 0; i < 100; ++i)
        {
            phList.push_back(
                boost::shared_ptr<ProcessFuture>(
                    pm->CreateProcess("/bin/sleep 1")));
        }
        foreach(const boost::shared_ptr<Forte::ProcessFuture> &ph, phList)
        {
            ASSERT_NO_THROW(ph->GetResult());
            ASSERT_EQ(0, ph->GetStatusCode());
            ASSERT_EQ(ProcessFuture::ProcessExited, ph->GetProcessTerminationType());
        }
        Forte::Timespec finish = Forte::MonotonicClock().GetTime();
        ASSERT_LT((finish - start), Forte::Timespec::FromSeconds(10));
    }
    catch (std::exception& e)
    {
        hlog(HLOG_ERR, "exception: %s", e.what());
        FAIL();
    }
}
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
