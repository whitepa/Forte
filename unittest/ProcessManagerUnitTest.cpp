
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
	
	boost::shared_ptr<ProcessHandle> ph2 = pm.CreateProcess("/bin/ls", "/", "/dev/null", "/home/tritchey/temp.out");
    ph2->Run();
    ph2->Wait();
    BOOST_CHECK(!ph2->IsRunning());
    hlog(HLOG_INFO, "Termination Type: %d", ph2->GetProcessTerminationType());
    hlog(HLOG_INFO, "StatusCode: %d", ph2->GetStatusCode());
}

BOOST_AUTO_TEST_CASE(Exceptions)
{
    hlog(HLOG_INFO, "Exceptions");
    ProcessManager pm;
    boost::shared_ptr<ProcessHandle> ph = pm.CreateProcess("/bin/sleep 3");
    BOOST_CHECK_THROW(ph->GetProcessTerminationType(), EProcessHandleProcessNotStarted);
    BOOST_CHECK_THROW(ph->GetStatusCode(), EProcessHandleProcessNotStarted);    
    BOOST_CHECK_THROW(ph->GetOutputString(), EProcessHandleProcessNotStarted);

    BOOST_CHECK_THROW(ph->Wait(), EProcessHandleProcessNotRunning);
    BOOST_CHECK_THROW(ph->Cancel(), EProcessHandleProcessNotRunning);
    BOOST_CHECK_THROW(ph->Abandon(), EProcessHandleProcessNotRunning);

    ph->Run();
    BOOST_CHECK_THROW(ph->GetProcessTerminationType(), EProcessHandleProcessNotFinished);
    BOOST_CHECK_THROW(ph->GetStatusCode(), EProcessHandleProcessNotFinished);    
    BOOST_CHECK_THROW(ph->GetOutputString(), EProcessHandleProcessNotFinished);

    BOOST_CHECK_THROW(ph->SetProcessCompleteCallback(NULL), EProcessHandleProcessStarted);
    BOOST_CHECK_THROW(ph->SetCurrentWorkingDirectory(""), EProcessHandleProcessStarted);
    BOOST_CHECK_THROW(ph->SetEnvironment(NULL), EProcessHandleProcessStarted);
    BOOST_CHECK_THROW(ph->SetInputFilename(""), EProcessHandleProcessStarted);
    BOOST_CHECK_THROW(ph->SetOutputFilename(""), EProcessHandleProcessStarted);
    BOOST_CHECK_THROW(ph->SetProcessManager(NULL), EProcessHandleProcessStarted);
    BOOST_CHECK_THROW(ph->Run(), EProcessHandleProcessStarted);

    ph->Wait();



}

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


////Boost Unit init function ///////////////////////////////////////////////////
test_suite*
init_unit_test_suite(int argc, char* argv[])
{

    logManager.SetLogMask("//stdout", HLOG_ALL);
    logManager.BeginLogging("//stdout");

    // initialize everything here

    // the boost test monitor will catch our attempts to kill
    // children, so we must set this option
    setenv("BOOST_TEST_CATCH_SYSTEM_ERRORS", "no", 1);

    return 0;
}
////////////////////////////////////////////////////////////////////////////////
