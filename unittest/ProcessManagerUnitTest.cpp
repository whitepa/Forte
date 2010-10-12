
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "ProcessManager.h"
#include "Process.h"

using namespace Forte;

LogManager logManager;

TEST(ProcessManager, RunProcess)
{
    logManager.BeginLogging("//stderr");
    try
    {
        hlog(HLOG_INFO, "new ProcessManager");
        boost::shared_ptr<ProcessManager> pm(new ProcessManager);
        hlog(HLOG_INFO, "CreateProcess");
        boost::shared_ptr<Process> ph = pm->CreateProcess("/bin/sleep 3");
        hlog(HLOG_INFO, "Run Process");
        ph->Run();
        hlog(HLOG_INFO, "Is Running");
        ASSERT_TRUE(ph->IsRunning());
        hlog(HLOG_INFO, "Wait");
        ph->Wait();
        hlog(HLOG_INFO, "Is Running");
        ASSERT_TRUE(!ph->IsRunning());
        hlog(HLOG_INFO, "Termination Type: %d", ph->GetProcessTerminationType());
        hlog(HLOG_INFO, "StatusCode: %d", ph->GetStatusCode());
        hlog(HLOG_INFO, "OutputString: %s", ph->GetOutputString().c_str());
    }
    catch (Exception &e)
    {
        hlog(HLOG_ERR, "exception: %s", e.what().c_str());
        FAIL();
    }
}

TEST(ProcessManager, Exceptions)
{
    try
    {
        hlog(HLOG_INFO, "Exceptions");
        boost::shared_ptr<ProcessManager> pm(new ProcessManager);
        hlog(HLOG_INFO, "CreateProcess");
        boost::shared_ptr<Process> ph = pm->CreateProcess("/bin/sleep 3");
        hlog(HLOG_INFO, "GetProcessTerminationType");
        ASSERT_THROW(ph->GetProcessTerminationType(), EProcessNotStarted);
        hlog(HLOG_INFO, "GetStatusCode");
        ASSERT_THROW(ph->GetStatusCode(), EProcessNotStarted);    
        hlog(HLOG_INFO, "GetOutputString");
        ASSERT_THROW(ph->GetOutputString(), EProcessNotStarted);

        hlog(HLOG_INFO, "Wait");
        ASSERT_THROW(ph->Wait(), EProcessNotRunning);
        hlog(HLOG_INFO, "Abandon");
        ASSERT_THROW(ph->Abandon(), EProcessNotRunning);

        ph->Run();
        ASSERT_THROW(ph->GetProcessTerminationType(), EProcessNotFinished);
        ASSERT_THROW(ph->GetStatusCode(), EProcessNotFinished);    
        ASSERT_THROW(ph->GetOutputString(), EProcessNotFinished);

        ASSERT_THROW(ph->SetProcessCompleteCallback(NULL), EProcessStarted);
        ASSERT_THROW(ph->SetCurrentWorkingDirectory(""), EProcessStarted);
        ASSERT_THROW(ph->SetEnvironment(NULL), EProcessStarted);
        ASSERT_THROW(ph->SetInputFilename(""), EProcessStarted);
        ASSERT_THROW(ph->SetOutputFilename(""), EProcessStarted);
        ASSERT_THROW(ph->Run(), EProcessStarted);

        ph->Wait();
    }
    catch (Exception &e)
    {
        hlog(HLOG_ERR, "Exception: %s", e.what().c_str());
        FAIL();
    }
    catch (std::exception &e)
    {
        hlog(HLOG_ERR, "std::exception: %s", e.what());
        FAIL();
    }
}

TEST(ProcessManager, CancelProcess)
{
    hlog(HLOG_INFO, "CancelProcess");
    boost::shared_ptr<ProcessManager> pm(new ProcessManager);
    boost::shared_ptr<Process> ph = pm->CreateProcess("/bin/sleep 100");
    ph->Run();
    ASSERT_TRUE(ph->IsRunning());
    ph->Signal(15);
    ph->Wait();
    ASSERT_TRUE(!ph->IsRunning());
    ASSERT_TRUE(ph->GetStatusCode() == 15);
    ASSERT_TRUE(ph->GetProcessTerminationType() == Forte::Process::ProcessKilled);
    hlog(HLOG_INFO, "OutputString: %s", ph->GetOutputString().c_str());
}


TEST(ProcessManager, AbandonProcess)
{
    hlog(HLOG_INFO, "AbandonProcess");
    boost::shared_ptr<ProcessManager> pm(new ProcessManager);
    boost::shared_ptr<Process> ph = pm->CreateProcess("/bin/sleep 100");
    ph->Run();
    ASSERT_TRUE(ph->IsRunning());
    ph->Abandon();
    ASSERT_THROW(ph->Wait(), EProcessHandleInvalid);
}

TEST(ProcessManager, FileIO)
{
    hlog(HLOG_INFO, "FileIO");
    boost::shared_ptr<ProcessManager> pm(new ProcessManager);
    boost::shared_ptr<Process> ph = pm->CreateProcess("/bin/sleep");

    // check that we propery throw when the input file doesn't exist
    ph->SetInputFilename("/foo/bar/baz");
    ASSERT_THROW(ph->Run(), EProcessUnableToOpenInputFile);

    // check that we properly throw when the output file can't be created
    ph = pm->CreateProcess("/bin/sleep");
    ph->SetOutputFilename("/foo/bar/baz");
    ASSERT_THROW(ph->Run(), EProcessUnableToOpenOutputFile);

    hlog(HLOG_DEBUG, "getting ready to try some output");

    // check that when we can create an output file that we can get 
    // the contents
    boost::shared_ptr<Process> ph2 = pm->CreateProcess("/bin/ls", "/", "temp.out");
    ph2->Run();
    ph2->Wait();
    ASSERT_TRUE(ph2->GetOutputString().find("proc") != string::npos);

    hlog(HLOG_DEBUG, "okay, now what happens when output is to /dev/null");

    // what happens if /dev/null is our output?
    boost::shared_ptr<Process> ph3 = pm->CreateProcess("/bin/ls", "/");
    ph3->Run();
    ph3->Wait();
    ASSERT_TRUE(ph3->GetOutputString().find("proc") == string::npos);

    hlog(HLOG_DEBUG, "reached end of function");

}

void ProcessComplete(boost::shared_ptr<Process> ph)
{
    hlog(HLOG_DEBUG, "process completion callback triggered");
    ASSERT_TRUE(ph->GetOutputString().find("proc") != string::npos);    
}

TEST(ProcessManager, Callbacks)
{
    hlog(HLOG_INFO, "Callbacks");
    boost::shared_ptr<ProcessManager> pm(new ProcessManager);
    boost::shared_ptr<Process> ph = pm->CreateProcess("/bin/ls", "/", "temp.out");
    ph->SetProcessCompleteCallback(ProcessComplete);
    ph->Run();
    ph->Wait();

}

////Boost Unit init function ///////////////////////////////////////////////////
// test_suite*
// init_unit_test_suite(int argc, char* argv[])
// {

//     logManager.SetLogMask("//stdout", HLOG_ALL);
//     logManager.BeginLogging("//stdout");

//     // initialize everything here

//     // the boost test monitor will catch our attempts to kill
//     // children, so we must set this option
//     setenv("BOOST_TEST_CATCH_SYSTEM_ERRORS", "no", 1);

//     return 0;
// }
////////////////////////////////////////////////////////////////////////////////
