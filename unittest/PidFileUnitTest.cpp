#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "PidFile.h"
#include <boost/make_shared.hpp>
#include "MockFileSystem.h"
#include "FTrace.h"

using namespace Forte;

LogManager logManager;

class PidFileTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");
    }
};

TEST_F(PidFileTest, IsProcessRunningReturnsFalseWhenProcessNotRunning)
{
    FTRACE;
    MockFileSystemPtr mfs = boost::make_shared<MockFileSystem>();

    // set up the mock file system
    mfs->SetFileExistsResult("/var/run/my.pid", true);
    mfs->FilePutContents("/var/run/my.pid", "5555", false);
    mfs->SetFileExistsResult("/proc/5555/cmdline", false);

    PidFile p(mfs, "/var/run/my.pid");
    ASSERT_FALSE(p.IsProcessRunning());
}

TEST_F(PidFileTest, IsProcessRunningReturnsTrueWhenProcessIsRunning)
{
    FTRACE;
    MockFileSystemPtr mfs = boost::make_shared<MockFileSystem>();

    // set up the mock file system
    mfs->SetFileExistsResult("/var/run/my.pid", true);
    mfs->FilePutContents("/var/run/my.pid", "5555", false);
    mfs->SetFileExistsResult("/proc/5555/cmdline", true);

    PidFile p(mfs, "/var/run/my.pid");
    ASSERT_TRUE(p.IsProcessRunning());
}

TEST_F(PidFileTest, UnlinkNotCalledWhenCreateNotCalled)
{
    FTRACE;
    MockFileSystemPtr mfs = boost::make_shared<MockFileSystem>();

    // set up the mock file system
    mfs->SetFileExistsResult("/var/run/my.pid", true);
    mfs->FilePutContents("/var/run/my.pid", "5555", false);
    mfs->SetFileExistsResult("/proc/5555/cmdline", true);

    {
        // need this in block so destructor is called before next assert
        PidFile p(mfs, "/var/run/my.pid");
        ASSERT_TRUE(p.IsProcessRunning());
    }
    ASSERT_FALSE(mfs->FileWasUnlinked("/var/run/my.pid"));
}

TEST_F(PidFileTest, CreateThrowsWhenProcessIsRunning)
{
    FTRACE;
    MockFileSystemPtr mfs = boost::make_shared<MockFileSystem>();

    // set up the mock file system
    mfs->SetFileExistsResult("/var/run/my.pid", true);
    mfs->FilePutContents("/var/run/my.pid", "5555", false);
    mfs->SetFileExistsResult("/proc/5555/cmdline", true);

    PidFile p(mfs, "/var/run/my.pid");
    ASSERT_THROW(p.Create(5555), EAlreadyRunning);
}

TEST_F(PidFileTest, CreateHappyPath)
{
    FTRACE;
    MockFileSystemPtr mfs = boost::make_shared<MockFileSystem>();

    // set up the mock file system
    mfs->SetFileExistsResult("/var/run/my.pid", false);;
    unsigned int pid = 5555;
    {
        PidFile p(mfs, "/var/run/my.pid");
        p.Create(pid);
    }
    FString pidFileContents(FStringFC(), "%u", pid);  // \n?
    ASSERT_STREQ(pidFileContents, mfs->FileGetContents("/var/run/my.pid"));
    ASSERT_TRUE(mfs->FileWasUnlinked("/var/run/my.pid"));
}
