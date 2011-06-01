
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "MockProcessManager.h"
#include "ProcessFuture.h"
#include "Clock.h"

using namespace Forte;

LogManager logManager;

class MockProcessManagerTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");
    }
    static void TearDownTestCase() {

    };
};

TEST_F(MockProcessManagerTest, HappyPath)
{
    boost::shared_ptr<MockProcessManager> mpm(new MockProcessManager);
    FString outputString = "i slept for 3 secs";
    mpm->SetCommandResponse("/bin/sleep 3", outputString, 0);
    
    boost::shared_ptr<ProcessFuture> ph = mpm->CreateProcess("/bin/sleep 3",
                                                             "./",
                                                             "sleepOutput.out");
    ASSERT_NO_THROW(ph->GetResult());
    ASSERT_TRUE(!ph->IsRunning());
    ASSERT_TRUE(ph->GetOutputString() == outputString);
}

TEST_F(MockProcessManagerTest, ProcessTimeout)
{
    boost::shared_ptr<MockProcessManager> mpm(new MockProcessManager);
    FString outputString = "i slept for 3 secs";
    mpm->SetCommandResponse("/bin/sleep 3", outputString, 0, 3000);
    MonotonicClock clock;
    
    boost::shared_ptr<ProcessFuture> ph = mpm->CreateProcess("/bin/sleep 3",
                                                             "./",
                                                             "sleepOutput.out");
    Timespec start = clock.GetTime();
    ASSERT_NO_THROW(ph->GetResult());
    Timespec stop = clock.GetTime();
    ASSERT_TRUE(!ph->IsRunning());
    ASSERT_TRUE(ph->GetOutputString() == outputString);
    hlog(HLOG_INFO, "time elapsed: %d", (stop.AsMillisec() - start.AsMillisec()));
    ASSERT_TRUE((stop.AsMillisec() - start.AsMillisec()) >= 3000);
}

TEST_F(MockProcessManagerTest, CommandNotSet)
{
    boost::shared_ptr<MockProcessManager> mpm(new MockProcessManager);
    ASSERT_THROW(mpm->CreateProcess(
                     "/bin/sleep 3"), 
                     EMockProcessManagerUnexpectedCommand);
}

TEST_F(MockProcessManagerTest, NonZeroStatus)
{
    boost::shared_ptr<MockProcessManager> mpm(new MockProcessManager);
    mpm->SetCommandResponse("/bin/sleep 3", "", 1);
    
    boost::shared_ptr<ProcessFuture> ph = mpm->CreateProcess("/bin/sleep 3");

    ASSERT_THROW(ph->GetResult(), EProcessFutureTerminatedWithNonZeroStatus);
    ASSERT_TRUE(!ph->IsRunning());
}

TEST_F(MockProcessManagerTest, MultipleProcesses)
{
    boost::shared_ptr<MockProcessManager> mpm(new MockProcessManager);
    mpm->SetCommandResponse("/bin/sleep 3", "", 0, 3000);
    mpm->SetCommandResponse("/bin/sleep 6", "", 0, 6000);
    
    boost::shared_ptr<ProcessFuture> ph1 = mpm->CreateProcess("/bin/sleep 3");
    boost::shared_ptr<ProcessFuture> ph2 = mpm->CreateProcess("/bin/sleep 6");

    ASSERT_NO_THROW(ph1->GetResult());
    ASSERT_TRUE(!ph1->IsRunning());

    ASSERT_NO_THROW(ph2->GetResult());
    ASSERT_TRUE(!ph2->IsRunning());
}


