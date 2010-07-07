
#include "boost/test/unit_test.hpp"
#include "LogManager.h"
#include "ProcessManager.h"
#include "ProcessHandler.h"

using namespace boost::unit_test;
using namespace Forte;

LogManager logManager;

BOOST_AUTO_TEST_CASE(RunProcess)
{
    hlog(HLOG_INFO, "CreateProcess");
    ProcessManager pm;
    boost::shared_ptr<ProcessHandler> ph = pm.CreateProcess("/bin/sleep 3");
    ph->Run();
    BOOST_CHECK(ph->IsRunning());
    ph->Wait();
    BOOST_CHECK(!ph->IsRunning());
    hlog(HLOG_INFO, "Termination Type: %d", ph->GetProcessTerminationType());
    hlog(HLOG_INFO, "StatusCode: %d", ph->GetStatusCode());
    hlog(HLOG_INFO, "OutputString: %s", ph->GetOutputString().c_str());
	
	hlog(HLOG_INFO, "Pausing...");
	sleep(3);
	
	ph = pm.CreateProcess("/bin/sleep 3");
    ph->Run();
    BOOST_CHECK(ph->IsRunning());
    ph->Wait();
    BOOST_CHECK(!ph->IsRunning());
    hlog(HLOG_INFO, "Termination Type: %d", ph->GetProcessTerminationType());
    hlog(HLOG_INFO, "StatusCode: %d", ph->GetStatusCode());
    hlog(HLOG_INFO, "OutputString: %s", ph->GetOutputString().c_str());
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
