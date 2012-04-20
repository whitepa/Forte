#include <gtest/gtest.h>

#include <boost/make_shared.hpp>
#include "FTrace.h"
#include "LogManager.h"
#include "Context.h"
#include "ProcFileSystem.h"
#include "MockFileSystem.h"
#include "FileSystem.h"
#include "FileSystemImpl.h"
#include "Foreach.h"

using namespace boost;
using namespace Forte;

LogManager logManager;

class ProcFileSystemUnitTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        //logManager.BeginLogging("//stdout", HLOG_ALL);
        hlog(HLOG_DEBUG, "Starting test...");
    }
};


TEST_F(ProcFileSystemUnitTest, UptimeMockContents)
{
    FTRACE;

    // setup
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/uptime", "30782.38 29768.69\n");

    // construct
    ProcFileSystem procFileSystem(fsptr);

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
    shared_ptr<FileSystem> fsptr(new FileSystemImpl());

    // construct
    ProcFileSystem procFileSystem(fsptr);

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
    ProcFileSystem procFileSystem(fsptr);

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

    shared_ptr<FileSystem> fsptr(new FileSystemImpl());
    ProcFileSystem procFileSystem(fsptr);

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

    ProcFileSystem procFileSystem(fsptr);

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

    ProcFileSystem procFileSystem(make_shared<FileSystemImpl>());

    vector<pid_t> pids;
    procFileSystem.PidOf("init", pids);
    EXPECT_EQ(1, pids.size());
    EXPECT_EQ(1, pids[0]);
}

TEST_F(ProcFileSystemUnitTest, GetProcessorInformationSuccess)
{
    // setup
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/cpuinfo", 
                           "processor	: 0\n"
                           "vendor_id	: GenuineIntel\n"
                           "cpu family	: 6\n"
                           "model		: 23\n"
                           "model name	: Intel(R) Core(TM)2 Duo CPU     T9600  @ 2.80GHz\n"
                           "stepping	: 10\n"
                           "cpu MHz		: 2800.000\n"
                           "cache size	: 6144 KB\n"
                           "physical id	: 0\n"
                           "siblings	: 2\n"
                           "core id		: 0\n"
                           "cpu cores	: 2\n"
                           "apicid		: 0\n"
                           "initial apicid	: 0\n"
                           "fpu		: yes\n"
                           "fpu_exception	: yes\n"
                           "cpuid level	: 13\n"
                           "wp		: yes\n"
                           "flags		: fpu vme de pse tsc msr pae mce cx8 apic mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx lm constant_tsc arch_perfmon rep_good aperfmperf unfair_spinlock pni ssse3 cx16 sse4_1 x2apic xsave hypervisor lahf_lm\n"
                           "bogomips	: 5600.00\n"
                           "clflush size	: 64\n"
                           "cache_alignment	: 64\n"
                           "address sizes	: 40 bits physical, 48 bits virtual\n"
                           "power management:\n"
                           "\n"
                           "processor	: 1\n"
                           "vendor_id	: GenuineIntel\n"
                           "cpu family	: 6\n"
                           "model		: 23\n"
                           "model name	: Intel(R) Core(TM)2 Duo CPU     T9600  @ 2.80GHz\n"
                           "stepping	: 10\n"
                           "cpu MHz		: 2800.000\n"
                           "cache size	: 6144 KB\n"
                           "physical id	: 0\n"
                           "siblings	: 2\n"
                           "core id		: 1\n"
                           "cpu cores	: 2\n"
                           "apicid		: 1\n"
                           "initial apicid	: 1\n"
                           "fpu		: yes\n"
                           "fpu_exception	: yes\n"
                           "cpuid level	: 13\n"
                           "wp		: yes\n"
                           "flags		: fpu vme de pse tsc msr pae mce cx8 apic mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx lm constant_tsc arch_perfmon rep_good aperfmperf unfair_spinlock pni ssse3 cx16 sse4_1 x2apic xsave hypervisor lahf_lm\n"
                           "bogomips	: 5600.00\n"
                           "clflush size	: 64\n"
                           "cache_alignment	: 64\n"
                           "address sizes	: 40 bits physical, 48 bits virtual\n"
                           "power management:\n");

    ProcFileSystem pfs(fsptr);
    
    std::vector<Forte::ProcFileSystem::CPUInfoPtr> processors;
    pfs.GetProcessorInformation(processors);

    ASSERT_EQ(2, processors.size());

    ASSERT_EQ(0, processors[0]->mProcessorNumber);
    ASSERT_EQ(1, processors[1]->mProcessorNumber);

    ASSERT_STREQ("GenuineIntel", processors[0]->mVendorId);
    ASSERT_STREQ("GenuineIntel", processors[1]->mVendorId);

    ASSERT_EQ(6, processors[0]->mCPUFamily);
    ASSERT_EQ(6, processors[1]->mCPUFamily);

    ASSERT_EQ(23, processors[0]->mModel);
    ASSERT_EQ(23, processors[1]->mModel);

    ASSERT_STREQ("Intel(R) Core(TM)2 Duo CPU     T9600  @ 2.80GHz",
                 processors[0]->mModelName);
    ASSERT_STREQ("Intel(R) Core(TM)2 Duo CPU     T9600  @ 2.80GHz",
                 processors[1]->mModelName);

    ASSERT_EQ(10, processors[0]->mStepping);
    ASSERT_EQ(10, processors[1]->mStepping);

    ASSERT_EQ(2800.000, processors[0]->mClockFrequencyInMegaHertz);
    ASSERT_EQ(2800.000, processors[1]->mClockFrequencyInMegaHertz);

    ASSERT_EQ(0, processors[0]->mPhysicalId);
    ASSERT_EQ(0, processors[1]->mPhysicalId);

    ASSERT_EQ(0, processors[0]->mCoreId);
    ASSERT_EQ(1, processors[1]->mCoreId);

    ASSERT_EQ(2, processors[0]->mNumberOfCores);
    ASSERT_EQ(2, processors[1]->mNumberOfCores);

    ASSERT_TRUE(processors[0]->mHasFPU);
    ASSERT_TRUE(processors[1]->mHasFPU);

    ASSERT_TRUE(processors[0]->mHasFPUException);
    ASSERT_TRUE(processors[1]->mHasFPUException);

    ASSERT_TRUE(processors[0]->HasFlag("lm"));
    ASSERT_TRUE(processors[1]->HasFlag("lm"));

    ASSERT_FALSE(processors[0]->HasFlag("foo"));
    ASSERT_FALSE(processors[1]->HasFlag("foo"));

    ASSERT_FALSE(processors[0]->SupportsVirtualization());
    ASSERT_FALSE(processors[1]->SupportsVirtualization());

    ASSERT_EQ(2, processors[0]->mSiblings);
    ASSERT_EQ(2, processors[1]->mSiblings);
}
