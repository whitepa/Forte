
#include "boost/test/unit_test.hpp"
#include "LogManager.h"
#include "ProcessManager.h"
#include "Process.h"

using namespace boost::unit_test;
using namespace Forte;

LogManager logManager;

BOOST_AUTO_TEST_CASE(RunProcess)
{
    try
    {
        hlog(HLOG_INFO, "new ProcessManager");
        boost::shared_ptr<ProcessManager> pm(new ProcessManager);
        hlog(HLOG_INFO, "CreateProcess");
        boost::shared_ptr<Process> ph = pm->CreateProcess("/bin/sleep 3");
        hlog(HLOG_INFO, "Run Process");
        ph->Run();
        hlog(HLOG_INFO, "Is Running");
        BOOST_CHECK(ph->IsRunning());
        hlog(HLOG_INFO, "Wait");
        ph->Wait();
        hlog(HLOG_INFO, "Is Running");
        BOOST_CHECK(!ph->IsRunning());
        hlog(HLOG_INFO, "Termination Type: %d", ph->GetProcessTerminationType());
        hlog(HLOG_INFO, "StatusCode: %d", ph->GetStatusCode());
        hlog(HLOG_INFO, "OutputString: %s", ph->GetOutputString().c_str());
    }
    catch (Exception &e)
    {
        hlog(HLOG_ERR, "%s", e.what().c_str());
        BOOST_FAIL("caught exception");
    }
}

BOOST_AUTO_TEST_CASE(Exceptions)
{
    hlog(HLOG_INFO, "Exceptions");
    ProcessManager pm;
    boost::shared_ptr<Process> ph = pm.CreateProcess("/bin/sleep 3");
    BOOST_CHECK_THROW(ph->GetProcessTerminationType(), EProcessNotStarted);
    BOOST_CHECK_THROW(ph->GetStatusCode(), EProcessNotStarted);    
    BOOST_CHECK_THROW(ph->GetOutputString(), EProcessNotStarted);

    BOOST_CHECK_THROW(ph->Wait(), EProcessNotRunning);
    BOOST_CHECK_THROW(ph->Abandon(), EProcessNotRunning);

    ph->Run();
    BOOST_CHECK_THROW(ph->GetProcessTerminationType(), EProcessNotFinished);
    BOOST_CHECK_THROW(ph->GetStatusCode(), EProcessNotFinished);    
    BOOST_CHECK_THROW(ph->GetOutputString(), EProcessNotFinished);

    BOOST_CHECK_THROW(ph->SetProcessCompleteCallback(NULL), EProcessStarted);
    BOOST_CHECK_THROW(ph->SetCurrentWorkingDirectory(""), EProcessStarted);
    BOOST_CHECK_THROW(ph->SetEnvironment(NULL), EProcessStarted);
    BOOST_CHECK_THROW(ph->SetInputFilename(""), EProcessStarted);
    BOOST_CHECK_THROW(ph->SetOutputFilename(""), EProcessStarted);
    BOOST_CHECK_THROW(ph->Run(), EProcessStarted);

    ph->Wait();

}

BOOST_AUTO_TEST_CASE(CancelProcess)
{
    hlog(HLOG_INFO, "CancelProcess");
    ProcessManager pm;
    boost::shared_ptr<Process> ph = pm.CreateProcess("/bin/sleep 100");
    ph->Run();
    BOOST_CHECK(ph->IsRunning());
    ph->Signal(15);
    ph->Wait();
    BOOST_CHECK(!ph->IsRunning());
    hlog(HLOG_INFO, "Termination Type: %d", ph->GetProcessTerminationType());
    hlog(HLOG_INFO, "StatusCode: %d", ph->GetStatusCode());
    hlog(HLOG_INFO, "OutputString: %s", ph->GetOutputString().c_str());
}


BOOST_AUTO_TEST_CASE(AbandonProcess)
{
    hlog(HLOG_INFO, "AbandonProcess");
    ProcessManager pm;
    boost::shared_ptr<Process> ph = pm.CreateProcess("/bin/sleep 100");
    ph->Run();
    BOOST_CHECK(ph->IsRunning());
    ph->Abandon();
    BOOST_CHECK_THROW(ph->Wait(), EProcessHandleInvalid);
}

BOOST_AUTO_TEST_CASE(FileIO)
{
    hlog(HLOG_INFO, "FileIO");
    ProcessManager pm;
    boost::shared_ptr<Process> ph = pm.CreateProcess("/bin/sleep");

    // check that we propery throw when the input file doesn't exist
    ph->SetInputFilename("/foo/bar/baz");
    BOOST_CHECK_THROW(ph->Run(), EProcessUnableToOpenInputFile);

    // check that we properly throw when the output file can't be created
    ph = pm.CreateProcess("/bin/sleep");
    ph->SetOutputFilename("/foo/bar/baz");
    BOOST_CHECK_THROW(ph->Run(), EProcessUnableToOpenOutputFile);

    hlog(HLOG_DEBUG, "getting ready to try some output");

    // check that when we can create an output file that we can get 
    // the contents
    boost::shared_ptr<Process> ph2 = pm.CreateProcess("/bin/ls", "/", "temp.out");
    ph2->Run();
    ph2->Wait();
    BOOST_CHECK(ph2->GetOutputString().find("proc") != string::npos);

    hlog(HLOG_DEBUG, "okay, now what happens when output is to /dev/null");

    // what happens if /dev/null is our output?
    boost::shared_ptr<Process> ph3 = pm.CreateProcess("/bin/ls", "/");
    ph3->Run();
    ph3->Wait();
    BOOST_CHECK(ph3->GetOutputString().find("proc") == string::npos);

    hlog(HLOG_DEBUG, "reached end of function");

}

void ProcessComplete(boost::shared_ptr<Process> ph)
{
    hlog(HLOG_DEBUG, "process completion callback triggered");
    BOOST_CHECK(ph->GetOutputString().find("proc") != string::npos);    
}

BOOST_AUTO_TEST_CASE(Callbacks)
{
    hlog(HLOG_INFO, "Callbacks");
    ProcessManager pm;
    boost::shared_ptr<Process> ph = pm.CreateProcess("/bin/ls", "/", "temp.out");
    ph->SetProcessCompleteCallback(ProcessComplete);
    ph->Run();
    ph->Wait();

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
