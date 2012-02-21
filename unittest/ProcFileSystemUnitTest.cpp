#include <gtest/gtest.h>

#include <boost/shared_ptr.hpp>
#include "FTrace.h"
#include "LogManager.h"
#include "Context.h"
#include "ProcFileSystem.h"
#include "MockFileSystem.h"
#include "FileSystem.h"
#include "Foreach.h"

using namespace boost;
using namespace Forte;

LogManager logManager;

class ProcFileSystemUnitTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        //logManager.BeginLogging(__FILE__ ".log");
        logManager.BeginLogging("//stdout");
        hlog(HLOG_DEBUG, "Starting test...");
    }

    Forte::Context mContext;
};


TEST_F(ProcFileSystemUnitTest, UptimeMockContents)
{
    FTRACE;

    // setup
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/uptime", "30782.38 29768.69\n");
    mContext.Set("forte.FileSystem", fsptr);

    // construct
    ProcFileSystem procFileSystem(mContext);

    // test uptime
    ProcFileSystem::Uptime uptime;
    procFileSystem.UptimeRead(uptime);

    EXPECT_EQ(uptime.mSecondsUp, 30782.38);
    EXPECT_EQ(uptime.mSecondsIdle, 29768.69);
}

TEST_F(ProcFileSystemUnitTest, UptimeVerifyRealOutput)
{
    FTRACE;

    // setup
    shared_ptr<FileSystem> fsptr(new FileSystem());
    mContext.Set("forte.FileSystem", fsptr);

    // construct
    ProcFileSystem procFileSystem(mContext);

    // test uptime
    ProcFileSystem::Uptime uptime;
    procFileSystem.UptimeRead(uptime);

    EXPECT_GT(uptime.mSecondsUp, 100); // just want to make sure
                                            // we got a number

    EXPECT_GT(uptime.mSecondsIdle, 100); //same
}

TEST_F(ProcFileSystemUnitTest, MemoryInfoReadMock)
{
    FTRACE;

    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/meminfo",
                           "MemTotal:      1015276 kB\n"
                           "MemFree:         16868 kB\n"
                           "Buffers:         68840 kB\n"
                           "Cached:         175200 kB\n"
                           "SwapCached:          0 kB\n"
                           "Active:         380376 kB\n"
                           "Inactive:        89132 kB\n"
                           "HighTotal:           0 kB\n"
                           "HighFree:            0 kB\n"
                           "LowTotal:      1015276 kB\n"
                           "LowFree:         16868 kB\n"
                           "SwapTotal:           0 kB\n"
                           "SwapFree:            0 kB\n"
                           "Dirty:             540 kB\n"
                           "Writeback:           0 kB\n"
                           "AnonPages:      224992 kB\n"
                           "Mapped:          33888 kB\n"
                           "Slab:           495432 kB\n"
                           "PageTables:       8884 kB\n"
                           "NFS_Unstable:        0 kB\n"
                           "Bounce:              0 kB\n"
                           "CommitLimit:    507636 kB\n"
                           "Committed_AS:  3019704 kB\n"
                           "VmallocTotal: 34359738367 kB\n"
                           "VmallocUsed:    268128 kB\n"
                           "VmallocChunk: 34359469547 kB\n"
                           "HugePages_Total:     0\n"
                           "HugePages_Free:      0\n"
                           "HugePages_Rsvd:      0\n"
                           "Hugepagesize:     2048 kB\n");
    mContext.Set("forte.FileSystem", fsptr);
    ProcFileSystem procFileSystem(mContext);

    Forte::StrDoubleMap memoryInfo;
    procFileSystem.MemoryInfoRead(memoryInfo);

    EXPECT_EQ(memoryInfo["MemTotal"], 1015276);
    EXPECT_EQ(memoryInfo["MemFree"], 16868);
    EXPECT_EQ(memoryInfo["Buffers"], 68840);
    EXPECT_EQ(memoryInfo["Cached"], 175200);
    EXPECT_EQ(memoryInfo["SwapCached"], 0);
    EXPECT_EQ(memoryInfo["Active"], 380376);
    EXPECT_EQ(memoryInfo["Inactive"], 89132);
    EXPECT_EQ(memoryInfo["HighTotal"], 0);
    EXPECT_EQ(memoryInfo["HighFree"], 0);
    EXPECT_EQ(memoryInfo["LowTotal"], 1015276);
    EXPECT_EQ(memoryInfo["LowFree"], 16868);
    EXPECT_EQ(memoryInfo["SwapTotal"], 0);
    EXPECT_EQ(memoryInfo["SwapFree"], 0);
    EXPECT_EQ(memoryInfo["Dirty"], 540);
    EXPECT_EQ(memoryInfo["Writeback"], 0);
    EXPECT_EQ(memoryInfo["AnonPages"], 224992);
    EXPECT_EQ(memoryInfo["Mapped"], 33888);
    EXPECT_EQ(memoryInfo["Slab"], 495432);
    EXPECT_EQ(memoryInfo["PageTables"], 8884);
    EXPECT_EQ(memoryInfo["NFS_Unstable"], 0);
    EXPECT_EQ(memoryInfo["Bounce"], 0);
    EXPECT_EQ(memoryInfo["CommitLimit"], 507636);
    EXPECT_EQ(memoryInfo["Committed_AS"], 3019704);
    EXPECT_EQ(memoryInfo["VmallocTotal"], 34359738367);
    EXPECT_EQ(memoryInfo["VmallocUsed"], 268128);
    EXPECT_EQ(memoryInfo["VmallocChunk"], 34359469547);
    EXPECT_EQ(memoryInfo["HugePages_Total"], 0);
    EXPECT_EQ(memoryInfo["HugePages_Free"], 0);
    EXPECT_EQ(memoryInfo["HugePages_Rsvd"], 0);
    EXPECT_EQ(memoryInfo["Hugepagesize"], 2048);
}


TEST_F(ProcFileSystemUnitTest, MemoryInfoReadReal)
{
    FTRACE;

    shared_ptr<FileSystem> fsptr(new FileSystem());
    mContext.Set("forte.FileSystem", fsptr);
    ProcFileSystem procFileSystem(mContext);

    Forte::StrDoubleMap memoryInfo;
    procFileSystem.MemoryInfoRead(memoryInfo);

    //spot check some values
    EXPECT_GT(memoryInfo["MemTotal"], 0);
    EXPECT_GT(memoryInfo["MemFree"], 0);

}


TEST_F(ProcFileSystemUnitTest, PidOfReturnsMatchingsPidsForProcess)
{
    FTRACE;

    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());

    fsptr->AddScanDirResult("/proc", "1");
    fsptr->AddScanDirResult("/proc", "2");
    fsptr->AddScanDirResult("/proc", "3");
    fsptr->AddScanDirResult("/proc", "4");
    fsptr->AddScanDirResult("/proc", "5");
    fsptr->AddScanDirResult("/proc", "cgroups");
    fsptr->AddScanDirResult("/proc", "cmdline");
    fsptr->AddScanDirResult("/proc", "cpuinfo");

    fsptr->FilePutContents("/proc/1/cmdline", "/sbin/init");
    fsptr->FilePutContents("/proc/2/cmdline", "/path/name\0args");
    fsptr->FilePutContents("/proc/3/cmdline", "/bin/path");
    fsptr->FilePutContents("/proc/4/cmdline", "/bin/sleep" "\0" "30");
    fsptr->FilePutContents("/proc/5/cmdline", "/bin/sleep" "\0" "40");

    fsptr->SetFileExistsResult("/proc/1/cmdline", true);
    fsptr->SetFileExistsResult("/proc/2/cmdline", true);
    fsptr->SetFileExistsResult("/proc/3/cmdline", true);
    fsptr->SetFileExistsResult("/proc/4/cmdline", true);
    fsptr->SetFileExistsResult("/proc/5/cmdline", true);

    mContext.Set("forte.FileSystem", fsptr);
    ProcFileSystem procFileSystem(mContext);

    vector<pid_t> pids;
    procFileSystem.PidOf("init", pids);
    EXPECT_EQ(1, pids.size());
    EXPECT_EQ(1, pids[0]);

    pids.clear();
    procFileSystem.PidOf("path", pids);
    EXPECT_EQ(1, pids.size());
    EXPECT_EQ(3, pids[0]);

    pids.clear();
    ASSERT_NO_THROW(procFileSystem.PidOf("sleep", pids));
    ASSERT_EQ(2, pids.size());
    EXPECT_EQ(4, pids[0]);
    EXPECT_EQ(5, pids[1]);

}

TEST_F(ProcFileSystemUnitTest, PidOfInitReturns1)
{
    FTRACE;

    shared_ptr<FileSystem> fsptr(new FileSystem());

    mContext.Set("forte.FileSystem", fsptr);

    ProcFileSystem procFileSystem(mContext);

    vector<pid_t> pids;
    procFileSystem.PidOf("init", pids);
    EXPECT_EQ(1, pids.size());
    EXPECT_EQ(1, pids[0]);
}
