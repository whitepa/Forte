
#include "boost/test/unit_test.hpp"
#include <boost/shared_ptr.hpp>

#include "LogManager.h"
#include "Context.h"
#include "ProcFileSystem.h"
#include "MockFileSystem.h"
#include "FileSystem.h"

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
