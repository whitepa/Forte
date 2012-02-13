
#include "boost/test/unit_test.hpp"
#include <boost/shared_ptr.hpp>

#include "LogManager.h"
#include "Context.h"
#include "ProcFileSystem.h"
#include "MockFileSystem.h"
#include "FileSystem.h"
#include "Foreach.h"

using namespace boost::unit_test;
using namespace boost;
using namespace Forte;

LogManager logManager;

BOOST_AUTO_TEST_CASE(UptimeMockContents)
{
    hlog(HLOG_INFO, "Uptime");
    Context c;
    // setup
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/uptime", "30782.38 29768.69\n");
    c.Set("forte.FileSystem", fsptr);

    // construct
    ProcFileSystem procFileSystem(c);

    // test uptime
    ProcFileSystem::Uptime uptime;
    procFileSystem.UptimeRead(uptime);

    BOOST_CHECK_EQUAL(uptime.mSecondsUp, 30782.38);
    BOOST_CHECK_EQUAL(uptime.mSecondsIdle, 29768.69);
}

BOOST_AUTO_TEST_CASE(UptimeVerifyRealOutput)
{
    hlog(HLOG_INFO, "UptimeVerifyRealOutput");
    Context c;
    // setup
    shared_ptr<FileSystem> fsptr(new FileSystem());
    c.Set("forte.FileSystem", fsptr);

    // construct
    ProcFileSystem procFileSystem(c);

    // test uptime
    ProcFileSystem::Uptime uptime;
    procFileSystem.UptimeRead(uptime);

    BOOST_CHECK_GT(uptime.mSecondsUp, 100); // just want to make sure
                                            // we got a number

    BOOST_CHECK_GT(uptime.mSecondsIdle, 100); //same
}

BOOST_AUTO_TEST_CASE(MemoryInfoReadMock)
{
    hlog(HLOG_INFO, "MemoryInfoReadMock");

    Context c;

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
    c.Set("forte.FileSystem", fsptr);
    ProcFileSystem procFileSystem(c);

    Forte::StrDoubleMap memoryInfo;
    procFileSystem.MemoryInfoRead(memoryInfo);

    BOOST_CHECK_EQUAL(memoryInfo["MemTotal"], 1015276);
    BOOST_CHECK_EQUAL(memoryInfo["MemFree"], 16868);
    BOOST_CHECK_EQUAL(memoryInfo["Buffers"], 68840);
    BOOST_CHECK_EQUAL(memoryInfo["Cached"], 175200);
    BOOST_CHECK_EQUAL(memoryInfo["SwapCached"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["Active"], 380376);
    BOOST_CHECK_EQUAL(memoryInfo["Inactive"], 89132);
    BOOST_CHECK_EQUAL(memoryInfo["HighTotal"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["HighFree"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["LowTotal"], 1015276);
    BOOST_CHECK_EQUAL(memoryInfo["LowFree"], 16868);
    BOOST_CHECK_EQUAL(memoryInfo["SwapTotal"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["SwapFree"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["Dirty"], 540);
    BOOST_CHECK_EQUAL(memoryInfo["Writeback"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["AnonPages"], 224992);
    BOOST_CHECK_EQUAL(memoryInfo["Mapped"], 33888);
    BOOST_CHECK_EQUAL(memoryInfo["Slab"], 495432);
    BOOST_CHECK_EQUAL(memoryInfo["PageTables"], 8884);
    BOOST_CHECK_EQUAL(memoryInfo["NFS_Unstable"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["Bounce"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["CommitLimit"], 507636);
    BOOST_CHECK_EQUAL(memoryInfo["Committed_AS"], 3019704);
    BOOST_CHECK_EQUAL(memoryInfo["VmallocTotal"], 34359738367);
    BOOST_CHECK_EQUAL(memoryInfo["VmallocUsed"], 268128);
    BOOST_CHECK_EQUAL(memoryInfo["VmallocChunk"], 34359469547);
    BOOST_CHECK_EQUAL(memoryInfo["HugePages_Total"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["HugePages_Free"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["HugePages_Rsvd"], 0);
    BOOST_CHECK_EQUAL(memoryInfo["Hugepagesize"], 2048);
}


BOOST_AUTO_TEST_CASE(MemoryInfoReadReal)
{
    hlog(HLOG_INFO, "MemoryInfoReadMock");

    Context c;
    shared_ptr<FileSystem> fsptr(new FileSystem());
    c.Set("forte.FileSystem", fsptr);
    ProcFileSystem procFileSystem(c);

    Forte::StrDoubleMap memoryInfo;
    procFileSystem.MemoryInfoRead(memoryInfo);

    //spot check some values
    BOOST_CHECK_GT(memoryInfo["MemTotal"], 0);
    BOOST_CHECK_GT(memoryInfo["MemFree"], 0);

}


////Boost Unit init function ///////////////////////////////////////////////////
test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    logManager.SetLogMask("//stdout", HLOG_ALL);
    logManager.BeginLogging("//stdout");

    // initialize everything here

    return 0;
}
////////////////////////////////////////////////////////////////////////////////
