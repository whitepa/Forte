#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "PidFile.h"
#include <boost/make_shared.hpp>
#include "GMockFileSystem.h"
#include "FTrace.h"

using namespace Forte;
using ::testing::_;
using ::testing::StrEq;
using ::testing::Throw;
using ::testing::Return;
using ::testing::StrictMock;

LogManager logManager;

class PidFileTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        //logManager.BeginLogging("//stdout");
        logManager.BeginLogging(__FILE__ ".log");
    }
};

TEST_F(PidFileTest, IsProcessRunningReturnsFalseWhenPidFileNotPresent)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(false));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_FALSE(p.IsProcessRunning());
}

TEST_F(PidFileTest, IsProcessRunningReturnsFalseWhenPidFileDoesntHaveProcFSEntry)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procCmdLine = "/proc/5555/cmdline";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, FileExists(StrEq(procCmdLine)))
        .WillRepeatedly(Return(false));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_FALSE(p.IsProcessRunning());
}

TEST_F(PidFileTest, IsProcessRunningReturnsFalseWhenProcFSEntryIsIncorrect)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procCmdLine = "/proc/5555/cmdline";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, FileExists(StrEq(procCmdLine)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(procCmdLine)))
        .WillRepeatedly(Return("/some/other/prog"));

    EXPECT_CALL(fs, Basename(StrEq("/some/other/prog"), _))
        .WillRepeatedly(Return("prog"));

    EXPECT_CALL(fs, Basename(StrEq("/usr/bin/my"), _))
        .WillRepeatedly(Return("my"));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_FALSE(p.IsProcessRunning());
}

TEST_F(PidFileTest, IsProcessRunningReturnsTrueWhenCorrectProcessIsRunning)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procCmdLine = "/proc/5555/cmdline";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, FileExists(StrEq(procCmdLine)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(procCmdLine)))
        .WillRepeatedly(Return("/other/path/to/my"));

    EXPECT_CALL(fs, Basename(StrEq("/other/path/to/my"), _))
        .WillRepeatedly(Return("my"));

    EXPECT_CALL(fs, Basename(StrEq("/usr/bin/my"), _))
        .WillRepeatedly(Return("my"));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_TRUE(p.IsProcessRunning());
}

TEST_F(PidFileTest, UnlinkNotCalledWhenCreateNotCalled)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<StrictMock<GMockFileSystem> >();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procCmdLine = "/proc/5555/cmdline";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, FileExists(StrEq(procCmdLine)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(procCmdLine)))
        .WillRepeatedly(Return("/other/path/to/my"));

    EXPECT_CALL(fs, Basename(StrEq("/other/path/to/my"), _))
        .WillRepeatedly(Return("my"));

    EXPECT_CALL(fs, Basename(StrEq("/usr/bin/my"), _))
        .WillRepeatedly(Return("my"));

    {
        PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
        ASSERT_TRUE(p.IsProcessRunning());
    }
    // StrictMock will assert if errant methods (Unlink) are called on
    // GMockFileSystem
}

TEST_F(PidFileTest, CreateThrowsWhenProcessIsRunning)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procCmdLine = "/proc/5555/cmdline";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, FileExists(StrEq(procCmdLine)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(procCmdLine)))
        .WillRepeatedly(Return("/other/path/to/my"));

    EXPECT_CALL(fs, Basename(StrEq("/other/path/to/my"), _))
        .WillRepeatedly(Return("my"));

    EXPECT_CALL(fs, Basename(StrEq("/usr/bin/my"), _))
        .WillRepeatedly(Return("my"));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_THROW(p.Create(5556), EAlreadyRunning);
}

TEST_F(PidFileTest, CreateHappyPath)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procCmdLine = "/proc/5555/cmdline";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(false));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs,
                FilePutContentsWithPerms(StrEq(pidfile), StrEq("5555"), _, _));

    EXPECT_CALL(fs, Unlink(StrEq(pidfile), _, _)).Times(2);

    unsigned int pid = 5555;
    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    p.Create(pid);
}


