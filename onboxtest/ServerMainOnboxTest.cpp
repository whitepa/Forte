#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "LogManager.h"
#include "ServerMain.h"
#include "FileSystemImpl.h"

#include "Forte.h"
#include "Context.h"

// #SCQAD TESTTAG: smoketest

using namespace Forte;
using namespace boost;

static const char* PIDLOCATION = "/var/run/ServerMainOnboxTest.pid";

class ServerMainOnboxTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        FileSystemImpl fs;
        if (!fs.FileExists(sConfigFile))
            sConfigFile = "/opt/scale/lib/qa/onbox/" + sConfigFile;
    }

    static void TearDownTestCase() {
    }

    void SetUp() {
    }

    void TearDown() {
    }

public:
    static Forte::FString sConfigFile;
};

Forte::FString ServerMainOnboxTest::sConfigFile = "ServerMainOnboxTest.info";

class TestServer : public ServerMain
{
public:
    TestServer(const Forte::FString &daemonName) :
        Forte::ServerMain(ServerMainOnboxTest::sConfigFile, HLOG_ALL, daemonName, false)
    {
    }

    virtual ~TestServer() {};
};

TEST_F(ServerMainOnboxTest, TestPIDFile)
{
    FTRACE;
    FileSystemImpl fs;

    int pid = getpid();
    FString cmdline = fs.FileGetContents(FString(FStringFC(),
                                                 "/proc/%d/cmdline", pid));
    size_t pos = cmdline.find_first_of('\0');
    FString processName = cmdline.Left(pos);

    hlog(HLOG_DEBUG, "Process name is '%s'", processName.c_str());
    shared_ptr<TestServer> server1;
    ASSERT_NO_THROW(server1 = make_shared<TestServer>(processName));
    ASSERT_EQ(getpid(), fs.FileGetContents(PIDLOCATION).Trim().AsInt32());
}

TEST_F(ServerMainOnboxTest, TestPIDFileThrows)
{
    FTRACE;
    FileSystemImpl fs;

    // parent is probably there
    int pid = getppid();

    cout << "Parent pid is " << pid << endl;
    FString cmdline = fs.FileGetContents(FString(FStringFC(), "/proc/%d/cmdline", pid));
    size_t pos = cmdline.find_first_of('\0');
    FString processName = cmdline.Left(pos);
    cout << "Processname is " << processName << endl;
    fs.FilePutContents(PIDLOCATION, FString(FStringFC(), "%d", pid), false, true);

    shared_ptr<TestServer> server1;
    ASSERT_THROW(server1 = make_shared<TestServer>(processName), EAlreadyRunning);
}

TEST_F(ServerMainOnboxTest, TestPIDFileDoesntThrowIfProcessDifferent)
{
    FTRACE;
    FileSystemImpl fs;

    // parent is probably there
    int parentPid = getppid();
    cout << "Parent pid is " << parentPid << endl;
    fs.FilePutContents(PIDLOCATION, FString(FStringFC(), "%d", parentPid), false, true);

    int pid = getpid();
    cout << "My pid is " << pid << endl;
    FString cmdline = fs.FileGetContents(FString(FStringFC(), "/proc/%d/cmdline", pid));
    size_t pos = cmdline.find_first_of('\0');
    FString processName = cmdline.Left(pos);

    shared_ptr<TestServer> server1;
    ASSERT_NO_THROW(server1 = make_shared<TestServer>(processName));
}
