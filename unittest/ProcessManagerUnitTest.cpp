
#include "boost/test/unit_test.hpp"
#include "LogManager.h"
#include "ProcessManager.h"

using namespace boost::unit_test;
using namespace Forte;

LogManager logManager;

BOOST_AUTO_TEST_CASE(RunProcess)
{
    hlog(HLOG_INFO, "CreateProcess");
    ProcessManager pm;
    ProcessHandler ph = pm.CreateProcess("/bin/sleep 10");
    ph.Run();
    BOOST_CHECK(ph.IsRunning());
    ph.Wait();
    BOOST_CHECK(!ph.IsRunning());
    FString out = ph.Output();
    hlog(HLOG_INFO, output);
    
    
}


/*
BOOST_AUTO_TEST_CASE(StatFSPathDoesNotExist)
{
    hlog(HLOG_INFO, "StatFSPathDoesNotExist");
    FileSystem f;
    struct statfs st;
    BOOST_CHECK_THROW(f.StatFS("pathdoesnotexist", &st), EFileSystemNoEnt);
}
*/

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
