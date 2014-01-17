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
using ::testing::AtLeast;

LogManager logManager;

class PidFileUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        //logManager.BeginLogging("//stdout");
        logManager.BeginLogging(__FILE__ ".log");
    }
};

TEST_F(PidFileUnitTest, IsProcessRunningWhenPidFileNotPresent)
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

TEST_F(PidFileUnitTest, IsProcessRunningWhenPidFileDoesntHaveProcFSEntries)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    const FString procBase = "/proc/5555/";
    FString procExe = procBase + "exe";
    FString procCmdLine = procBase + "cmdline";
    FString procStat = procBase + "stat";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, Basename(StrEq("/usr/bin/my"), _))
        .WillRepeatedly(Return("my"));

    EXPECT_CALL(fs, FileExists(StrEq(procExe)))
        .WillRepeatedly(Return(false));

    EXPECT_CALL(fs, FileExists(StrEq(procCmdLine)))
        .WillRepeatedly(Return(false));

    EXPECT_CALL(fs, FileExists(StrEq(procStat)))
        .WillRepeatedly(Return(false));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_FALSE(p.IsProcessRunning());
}

TEST_F(PidFileUnitTest, IsProcessRunningWhenProcFSEntriesExistButIncorrect)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    const FString procBase = "/proc/5555/";
    FString procExe = procBase + "exe";
    FString procCmdLine = procBase + "cmdline";
    FString procStat = procBase + "stat";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, Basename(StrEq("/usr/bin/my"), _))
        .WillRepeatedly(Return("my"));

    EXPECT_CALL(fs, FileExists(StrEq(procExe)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, ResolveSymLink(StrEq(procExe)))
        .WillRepeatedly(Return("/some/other/prog"));

    EXPECT_CALL(fs, FileExists(StrEq(procCmdLine)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(procCmdLine)))
        .WillRepeatedly(Return("whacky cmdline"));

    EXPECT_CALL(fs, FileExists(StrEq(procStat)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(procStat)))
        .WillRepeatedly(Return("1234 (otherProg) gobbly gook"));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_FALSE(p.IsProcessRunning());
}

TEST_F(PidFileUnitTest, IsProcessRunningWhenSymlinkResolvesCorrectly)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procExe = "/proc/5555/exe";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, FileExists(StrEq(procExe)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, ResolveSymLink(StrEq(procExe)))
        .WillRepeatedly(Return("/other/path/to/my"));

    EXPECT_CALL(fs, Basename(StrEq("/other/path/to/my"), _))
        .WillRepeatedly(Return("my"));

    EXPECT_CALL(fs, Basename(StrEq("/usr/bin/my"), _))
        .WillRepeatedly(Return("my"));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_TRUE(p.IsProcessRunning());
}

TEST_F(PidFileUnitTest, UnlinkNotCalledWhenCreateNotCalled)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<StrictMock<GMockFileSystem> >();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procExe = "/proc/5555/exe";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, FileExists(StrEq(procExe)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, ResolveSymLink(StrEq(procExe)))
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

TEST_F(PidFileUnitTest, CreateThrowsWhenProcessIsRunning)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procExe = "/proc/5555/exe";

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, FileExists(StrEq(procExe)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, ResolveSymLink(StrEq(procExe)))
        .WillRepeatedly(Return("/other/path/to/my"));

    EXPECT_CALL(fs, Basename(StrEq("/other/path/to/my"), _))
        .WillRepeatedly(Return("my"));

    EXPECT_CALL(fs, Basename(StrEq("/usr/bin/my"), _))
        .WillRepeatedly(Return("my"));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_THROW(p.Create(5556), EAlreadyRunning);
}

TEST_F(PidFileUnitTest, CreateHappyPath)
{
    FTRACE;
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procExe = "/proc/5555/exe";

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

TEST_F(PidFileUnitTest, IsProcessRunningProcCmdLineFallback)
{
    FTRACE;
    logManager.BeginLogging("//stdout");
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procCmdLine = "/proc/5555/cmdline";

    EXPECT_CALL(fs, Basename(StrEq("/usr/bin/my"), _))
        .WillRepeatedly(Return("my"));

    EXPECT_CALL(fs, FileExists(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, FileExists(StrEq(procCmdLine)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(procCmdLine)))
        .WillRepeatedly(Return("/other/path/to/my"));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_TRUE(p.IsProcessRunning());
}

TEST_F(PidFileUnitTest, IsProcessRunningProcStatFallback)
{
    FTRACE;
    logManager.BeginLogging("//stdout");
    GMockFileSystemPtr mfs = boost::make_shared<GMockFileSystem>();
    GMockFileSystem &fs(*mfs);

    FString pidfile = "/var/run/my.pid";
    FString procStat = "/proc/5555/stat";

    EXPECT_CALL(fs, Basename(StrEq("/usr/bin/my"), _))
        .WillRepeatedly(Return("my"));

    EXPECT_CALL(fs, FileExists(_))
        .Times(AtLeast(2))
        .WillRepeatedly(Return(false));

    EXPECT_CALL(fs, FileExists(StrEq(pidfile)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(pidfile)))
        .WillRepeatedly(Return("5555"));

    EXPECT_CALL(fs, FileExists(StrEq(procStat)))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(fs, FileGetContents(StrEq(procStat)))
        .WillRepeatedly(Return("5555 (my) gobbly goop"));

    PidFile p(mfs, "/var/run/my.pid", "/usr/bin/my");
    ASSERT_TRUE(p.IsProcessRunning());
}
