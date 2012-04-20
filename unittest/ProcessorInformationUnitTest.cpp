#include <gtest/gtest.h>

#include <boost/make_shared.hpp>
#include "FTrace.h"
#include "LogManager.h"
#include "Context.h"
#include "ProcessorInformation.h"
#include "MockFileSystem.h"
#include "FileSystem.h"
#include "FileSystemImpl.h"
#include "Foreach.h"

using namespace boost;
using namespace Forte;

LogManager logManager;

class ProcessorInformationUnitTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        hlog(HLOG_DEBUG, "Starting test...");
    }
};


TEST_F(ProcessorInformationUnitTest, SingleSocketQuadCoreHT)
{
    FTRACE;

    // setup
    FileSystemImpl fs;
    FString cpuinfoContents = fs.FileGetContents("./cpuinfo/sample1.cpuinfo");
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/cpuinfo", cpuinfoContents);

    // construct
    ProcFileSystemPtr pfs = ProcFileSystemPtr(new ProcFileSystem(fsptr));
    ProcessorInformation pi(pfs);

    ASSERT_EQ(1, pi.GetNumberOfSockets());
    ASSERT_EQ(4, pi.GetTotalNumberOfCores());
    ASSERT_EQ(1, pi.GetNumberOfPhysicalCPUs());
    ASSERT_EQ(2666749000, pi.GetClockFrequencyInHertz());
    ASSERT_EQ(8, pi.GetTotalNumberOfHardwareThreads());
}

TEST_F(ProcessorInformationUnitTest, SingleSocketSingleCoreNoHT)
{
    FTRACE;

    // setup
    FileSystemImpl fs;
    FString cpuinfoContents = fs.FileGetContents("./cpuinfo/sample1_1socket1coreNoHT.cpuinfo");
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/cpuinfo", cpuinfoContents);

    // construct
    ProcFileSystemPtr pfs = ProcFileSystemPtr(new ProcFileSystem(fsptr));
    ProcessorInformation pi(pfs);

    ASSERT_EQ(1, pi.GetNumberOfSockets());
    ASSERT_EQ(1, pi.GetTotalNumberOfCores());
    ASSERT_EQ(1, pi.GetNumberOfPhysicalCPUs());
    ASSERT_EQ(1, pi.GetTotalNumberOfHardwareThreads());
}

TEST_F(ProcessorInformationUnitTest, SingleSocketSingleCoreHT)
{
    FTRACE;

    // setup
    FileSystemImpl fs;
    FString cpuinfoContents = fs.FileGetContents("./cpuinfo/sample2_1socket1coreHT.cpuinfo");
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/cpuinfo", cpuinfoContents);

    // construct
    ProcFileSystemPtr pfs = ProcFileSystemPtr(new ProcFileSystem(fsptr));
    ProcessorInformation pi(pfs);

    ASSERT_EQ(1, pi.GetNumberOfSockets());
    ASSERT_EQ(1, pi.GetTotalNumberOfCores());
    ASSERT_EQ(1, pi.GetNumberOfPhysicalCPUs());
    ASSERT_EQ(2, pi.GetTotalNumberOfHardwareThreads());
}

TEST_F(ProcessorInformationUnitTest, SingleSocketQuadCore)
{
    FTRACE;

    // setup
    FileSystemImpl fs;
    FString cpuinfoContents = fs.FileGetContents("./cpuinfo/sample3_1socket4core.cpuinfo");
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/cpuinfo", cpuinfoContents);

    // construct
    ProcFileSystemPtr pfs = ProcFileSystemPtr(new ProcFileSystem(fsptr));
    ProcessorInformation pi(pfs);

    ASSERT_EQ(1, pi.GetNumberOfSockets());
    ASSERT_EQ(4, pi.GetTotalNumberOfCores());
    ASSERT_EQ(1, pi.GetNumberOfPhysicalCPUs());
    ASSERT_EQ(4, pi.GetTotalNumberOfHardwareThreads());
}

TEST_F(ProcessorInformationUnitTest, SingleSocketDualCore)
{
    FTRACE;

    // setup
    FileSystemImpl fs;
    FString cpuinfoContents = fs.FileGetContents("./cpuinfo/sample3a_1socket2core.cpuinfo");
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/cpuinfo", cpuinfoContents);

    // construct
    ProcFileSystemPtr pfs = ProcFileSystemPtr(new ProcFileSystem(fsptr));
    ProcessorInformation pi(pfs);

    ASSERT_EQ(1, pi.GetNumberOfSockets());
    ASSERT_EQ(2, pi.GetTotalNumberOfCores());
    ASSERT_EQ(1, pi.GetNumberOfPhysicalCPUs());
    ASSERT_EQ(2, pi.GetTotalNumberOfHardwareThreads());
}

TEST_F(ProcessorInformationUnitTest, DualSocketSingleCoreHT)
{
    FTRACE;

    // setup
    FileSystemImpl fs;
    FString cpuinfoContents = fs.FileGetContents("./cpuinfo/sample4_2socket1coreHT.cpuinfo");
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/cpuinfo", cpuinfoContents);

    // construct
    ProcFileSystemPtr pfs = ProcFileSystemPtr(new ProcFileSystem(fsptr));
    ProcessorInformation pi(pfs);

    ASSERT_EQ(2, pi.GetNumberOfSockets());
    ASSERT_EQ(2, pi.GetTotalNumberOfCores());
    ASSERT_EQ(2, pi.GetNumberOfPhysicalCPUs());
    ASSERT_EQ(4, pi.GetTotalNumberOfHardwareThreads());
}

TEST_F(ProcessorInformationUnitTest, DualSocketDualCoreNoHT)
{
    FTRACE;

    // setup
    FileSystemImpl fs;
    FString cpuinfoContents = fs.FileGetContents("./cpuinfo/sample5_2socket2coreNoHT.cpuinfo");
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/cpuinfo", cpuinfoContents);

    // construct
    ProcFileSystemPtr pfs = ProcFileSystemPtr(new ProcFileSystem(fsptr));
    ProcessorInformation pi(pfs);

    ASSERT_EQ(2, pi.GetNumberOfSockets());
    ASSERT_EQ(4, pi.GetTotalNumberOfCores());
    ASSERT_EQ(2, pi.GetNumberOfPhysicalCPUs());
    ASSERT_EQ(4, pi.GetTotalNumberOfHardwareThreads());
}

