
#include "boost/test/unit_test.hpp"
#include "LogManager.h"
#include "ProcessManager.h"
#include "ProcessHandle.h"

using namespace boost::unit_test;
using namespace Forte;

LogManager logManager;

BOOST_AUTO_TEST_CASE(RunProcess)
{
    hlog(HLOG_INFO, "CreateProcess");
    ProcessManager pm;
    boost::shared_ptr<ProcessHandle> ph = pm.CreateProcess("/bin/sleep 3");
    ph->Run();
    BOOST_CHECK(ph->IsRunning());
    ph->Wait();
    BOOST_CHECK(!ph->IsRunning());
    hlog(HLOG_INFO, "Termination Type: %d", ph->GetProcessTerminationType());
    hlog(HLOG_INFO, "StatusCode: %d", ph->GetStatusCode());
    hlog(HLOG_INFO, "OutputString: %s", ph->GetOutputString().c_str());
	
	hlog(HLOG_INFO, "Pausing...");
	sleep(3);
	
	boost::shared_ptr<ProcessHandle> ph2 = pm.CreateProcess("/bin/ls", "/", "/dev/null", "/home/tritchey/temp.out");
    ph2->Run();
    BOOST_CHECK(ph2->IsRunning());
    ph2->Wait();
    BOOST_CHECK(!ph2->IsRunning());
    hlog(HLOG_INFO, "Termination Type: %d", ph2->GetProcessTerminationType());
    hlog(HLOG_INFO, "StatusCode: %d", ph2->GetStatusCode());
    hlog(HLOG_INFO, "OutputString: %s", ph2->GetOutputString().c_str());
}

// this must be run with --catch_system_errors=no
 /*
BOOST_AUTO_TEST_CASE(CancelProcess)
{
    hlog(HLOG_INFO, "CancelProcess");
    ProcessManager pm;
    boost::shared_ptr<ProcessHandle> ph = pm.CreateProcess("/bin/sleep 100");
    ph->Run();
    BOOST_CHECK(ph->IsRunning());
    ph->Cancel();
    BOOST_CHECK(!ph->IsRunning());
    hlog(HLOG_INFO, "Termination Type: %d", ph->GetProcessTerminationType());
    hlog(HLOG_INFO, "StatusCode: %d", ph->GetStatusCode());
    hlog(HLOG_INFO, "OutputString: %s", ph->GetOutputString().c_str());
	
}

BOOST_AUTO_TEST_CASE(AbandonProcess)
{
    hlog(HLOG_INFO, "AbandonProcess");
    ProcessManager pm;
    boost::shared_ptr<ProcessHandle> ph = pm.CreateProcess("/bin/sleep 100");
    ph->Run();
    BOOST_CHECK(ph->IsRunning());
    ph->Abandon();
	
	ph = pm.CreateProcess("/bin/sleep 100");
    ph->Run();
    BOOST_CHECK(ph->IsRunning());
    ph->Abandon(true);
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
