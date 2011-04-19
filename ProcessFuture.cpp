// ProcessFuture.cpp

#include "ProcessFuture.h"
#include "ProcessManager.h"
#include "AutoMutex.h"
#include "Exception.h"
#include "LogManager.h"
#include "LogTimer.h"
#include "ServerMain.h"
#include "Util.h"
#include "FTrace.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <csignal>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <boost/algorithm/string.hpp>


using namespace boost;
using namespace Forte;

Forte::ProcessFuture::ProcessFuture(const boost::shared_ptr<ProcessManager> &mgr,
                        const FString &procmon,
                        const FString &command, 
                        const FString &currentWorkingDirectory, 
                        const FString &outputFilename, 
                        const FString &inputFilename, 
                        const StrStrMap *environment) : 
    mProcessManagerPtr(mgr),
    mProcmonPath(procmon),
    mCommand(command), 
    mCurrentWorkingDirectory(currentWorkingDirectory),
    mOutputFilename(outputFilename),
    mInputFilename(inputFilename),
    mMonitorPid(-1),
    mProcessPid(-1),
    mOutputString(""),
    mState(STATE_READY),
    mWaitCond(mWaitLock)
{
    // copy the environment entries
    if(environment) 
    {
        mEnvironment.insert(environment->begin(), environment->end());
    }
}

Forte::ProcessFuture::~ProcessFuture() 
{
    try
    {
        abandon();
    }
    catch (Exception &e)
    {
        hlog(HLOG_ERR, "%s", e.what());
    }
    catch (std::exception &e)
    {
        hlog(HLOG_ERR, "%s", e.what());
    }
    catch (...)
    {
        hlog(HLOG_ERR, "Unknown exception!");
    }
}

void Forte::ProcessFuture::SetProcessCompleteCallback(ProcessCompleteCallback processCompleteCallback)
{
    if(mState != STATE_READY) 
    {
        hlog(HLOG_ERR, "tried setting the process complete callback after the process had been started");
        throw EProcessFutureStarted();
    }
    mProcessCompleteCallback = processCompleteCallback;
}

void Forte::ProcessFuture::SetCurrentWorkingDirectory(const FString &cwd) 
{
    if(mState != STATE_READY) 
    {
        hlog(HLOG_ERR, "tried setting the current working directory after the process had been started");
        throw EProcessFutureStarted();
    }
    mCurrentWorkingDirectory = cwd;
}

void Forte::ProcessFuture::SetEnvironment(const StrStrMap *env)
{
    if(mState != STATE_READY)
    {
        hlog(HLOG_ERR, "tried setting the environment after the process had been started");
        throw EProcessFutureStarted();
    }
    if(env) 
    {
        mEnvironment.clear();
        mEnvironment.insert(env->begin(), env->end());
    }
}

void Forte::ProcessFuture::SetInputFilename(const FString &infile)
{
    if(mState != STATE_READY)
    {
        hlog(HLOG_ERR, "tried setting the input filename after the process had been started");
        throw EProcessFutureStarted();
    }
    mInputFilename = infile;
}

void Forte::ProcessFuture::SetOutputFilename(const FString &outfile)
{
    if(mState != STATE_READY)
    {
        hlog(HLOG_ERR, "tried setting the output filename after the process had been started");
        throw EProcessFutureStarted();
    }
    mOutputFilename = outfile;
}

boost::shared_ptr<ProcessManager> Forte::ProcessFuture::GetProcessManager(void)
{
    shared_ptr<ProcessManager> mgr(mProcessManagerPtr.lock());
    if (!mgr)
    {
        throw EProcessFutureHandleInvalid();
    }
    return mgr;
}

void Forte::ProcessFuture::run()
{
    if (!mManagementChannel)
        throw EProcessFutureHandleInvalid();

    if (mState != STATE_READY)
        throw EProcessFutureStarted();

    // we must set the state prior to sending the start PDU to avoid a race
    setState(STATE_STARTING);

    try
    {

        // send the param PDUs, with full command line info, etc
        PDU paramPDU(ProcessOpParam, sizeof(ProcessParamPDU));
        ProcessParamPDU *param = reinterpret_cast<ProcessParamPDU*>(paramPDU.payload);

        // \TODO safe copy of these strings, ensure null termination,
        // disallow truncation (throw exception if the source strings are
        // too long)
        strncpy(param->str, mCommand.c_str(), sizeof(param->str));
        param->param = ProcessCmdline;
        mManagementChannel->SendPDU(paramPDU);

        strncpy(param->str, mCurrentWorkingDirectory.c_str(), sizeof(param->str));
        param->param = ProcessCwd;
        mManagementChannel->SendPDU(paramPDU);
    
        strncpy(param->str, mInputFilename.c_str(), sizeof(param->str));
        param->param = ProcessInfile;
        mManagementChannel->SendPDU(paramPDU);
    
        strncpy(param->str, mOutputFilename.c_str(), sizeof(param->str));
        param->param = ProcessOutfile;
        mManagementChannel->SendPDU(paramPDU);

        // send the control PDU telling the process to start
        PDU pdu(ProcessOpControlReq, sizeof(ProcessControlReqPDU));
        ProcessControlReqPDU *control = reinterpret_cast<ProcessControlReqPDU*>(pdu.payload);
        control->control = ProcessControlStart;
        mManagementChannel->SendPDU(pdu);
    }
    catch (Exception &e)
    {
        throw EProcessFutureManagementProcFailed(e.what());
    }
    // wait for the process to change state
    AutoUnlockMutex lock(mWaitLock);
    while (mState == STATE_STARTING)
        mWaitCond.TimedWait(1);

}

void Forte::ProcessFuture::setState(int state)
{
    mState = state;
    if (isInTerminalState())
    {
        if (mProcessCompleteCallback)
        {
            mProcessCompleteCallback(static_pointer_cast<ProcessFuture>(shared_from_this()));
        }

        // set the future
        try
        {
            switch (mState)
            {
            case STATE_EXITED:
            {
                // first check the exit code is 0
                if (mStatusCode == 0)
                {
                    // set the future as completed fine
                    SetException(boost::exception_ptr());
                }
                else 
                {
                    // throw a boost::throw with non-zero status code
                    boost::throw_exception(
                        EProcessFutureTerminatedWithNonZeroStatus(
                            FStringFC(), "%u", mStatusCode)
                        );
                }
                break;
            }
        
            case STATE_KILLED:
            {
                // throw exception saying the process was killed
                boost::throw_exception(EProcessFutureKilled());
                break;
            }

            case STATE_ERROR:
            {
                switch (mStatusCode)
                {
                case ProcessUnableToOpenInputFile:
                    boost::throw_exception(
                        EProcessFutureUnableToOpenInputFile(mErrorString));
                    break;
                case ProcessUnableToOpenOutputFile:
                    boost::throw_exception(
                        EProcessFutureUnableToOpenOutputFile(mErrorString));
                    break;  
                case ProcessUnableToCWD:
                    boost::throw_exception(
                        EProcessFutureUnableToCWD(mErrorString));
                    break;
                case ProcessUnableToFork:
                    boost::throw_exception(
                        EProcessFutureUnableToFork(mErrorString));
                    break;
                case ProcessUnableToExec:
                    boost::throw_exception(
                        EProcessFutureUnableToExec(mErrorString));
                    break;
                case ProcessProcmonFailure:
                    boost::throw_exception(
                        EProcessFutureManagementProcFailed());
                    break;
                default:
                    boost::throw_exception(
                        EProcessFuture(mErrorString));
                    break;
                }                    
                break;
            }

            case STATE_ABANDONED:
            {
                boost::throw_exception(
                    EProcessFutureAbandoned());
            }
        
            default:
            {
                // throw exception saying the process ended due to unknown reasons
                boost::throw_exception(
                    EProcessFutureTerminatedDueToUnknownReason());
            }
            }
        }
        catch (...)
        {
            // we got an exception which we need to 
            // record in the future
            SetException(boost::current_exception());
        }

    }

    AutoUnlockMutex lock(mWaitLock);
    mWaitCond.Broadcast();
}

void Forte::ProcessFuture::startMonitor()
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
        throw EProcessFutureUnableToCreateSocket(FStringFC(), "%s", strerror(errno));
    AutoFD parentfd(fds[0]);
    AutoFD childfd(fds[1]);

    pid_t childPid = fork();
    if(childPid < 0) 
        throw EProcessFutureUnableToFork(FStringFC(), "%s", strerror(errno));
    else if(childPid == 0)
    {
        fprintf(stderr, "procmon child, childfd=%d\n", childfd.GetFD());
        // child
        // close all file descriptors, except the PDU channel
        while (close(parentfd) == -1 && errno == EINTR);
        for (int n = 0; n < 1024; ++n)
            if (n != childfd)
                while (close(n) == -1 && errno == EINTR);
        // clear sig mask
        sigset_t set;
        sigemptyset(&set);
        pthread_sigmask(SIG_SETMASK, &set, NULL);

        // create a new process group / session
        daemon(1, 0);
        setsid();
        FString childfdStr(childfd);
        char **vargs = new char* [3];
        vargs[0] = const_cast<char *>("(procmon)"); // \TODO include the name of the monitored process
        vargs[1] = const_cast<char *>(childfdStr.c_str());
        vargs[2] = 0;
//        fprintf(stderr, "procmon child, exec '%s' '%s'\n", mProcmonPath.c_str(), vargs[1]);
        execv(mProcmonPath, vargs);
//        fprintf(stderr, "procmon child, exec failed: %d %s\n", errno, strerror(errno));
        exit(-1);
    }
    else
    {
        // parent
        childfd.Close();
        // add a PDUPeer to the PeerSet owned by the ProcessManager
        shared_ptr<ProcessManager> pm(mProcessManagerPtr.lock());
        mManagementChannel = pm->addPeer(parentfd);
        parentfd.Release();
    }
}

void Forte::ProcessFuture::GetResult()
{
    if(mState == STATE_READY) 
    {
        hlog(HLOG_ERR, "tried waiting on a process that has not been started");
        throw EProcessFutureNotRunning();
    }

    Future<void>::GetResult();
}


void Forte::ProcessFuture::abandon()
{

    hlog(HLOG_DEBUG, "abandoning process ID %u", mProcessPid);

    if (mManagementChannel)
    {
        GetProcessManager()->abandonProcess(mManagementChannel->GetFD());
        while (close(mManagementChannel->GetFD()) == -1 && errno == EINTR);
    }
    
    setState(STATE_ABANDONED);
}

bool Forte::ProcessFuture::IsRunning()
{
    return isInRunningState();
}

void Forte::ProcessFuture::Signal(int signum)
{
    if (!isInRunningState())
        throw EProcessFutureNotRunning();

    PDU pdu(ProcessOpControlReq, sizeof(ProcessControlReqPDU));
    ProcessControlReqPDU *control = reinterpret_cast<ProcessControlReqPDU*>(pdu.payload);
    control->control = ProcessControlSignal;
    control->signum = signum;
    mManagementChannel->SendPDU(pdu);
}

unsigned int Forte::ProcessFuture::GetStatusCode() 
{ 
    if(mState == STATE_READY)
    {
        hlog(HLOG_ERR, "tried grabbing the status code from a process that hasn't been started");
        throw EProcessFutureNotStarted();
    }
    else if (!isInTerminalState())
    {
        hlog(HLOG_ERR, "tried grabbing the status code from a process that hasn't completed yet");
        throw EProcessFutureNotFinished();
    }
    return mStatusCode;
}

Forte::ProcessFuture::ProcessTerminationType Forte::ProcessFuture::GetProcessTerminationType() 
{ 
    if(mState == STATE_READY)
    {
        hlog(HLOG_ERR, "tried grabbing the termination type from a process that hasn't been started");
        throw EProcessFutureNotStarted();
    }
    else if (!isInTerminalState())
    {
        hlog(HLOG_ERR, "tried grabbing the termination type from a process that hasn't completed yet");
        throw EProcessFutureNotFinished();
    }
    else if (mState == STATE_EXITED)
        return ProcessExited;
    else if (mState == STATE_KILLED)
        return ProcessKilled;
    else // STATE_ERROR or other
        return ProcessUnknownTermination;
}

FString Forte::ProcessFuture::GetOutputString()
{
    if(mState == STATE_READY)
    {
        hlog(HLOG_ERR, "tried grabbing the output from a process that hasn't been started");
        throw EProcessFutureNotStarted();
    }
    else if(!isInTerminalState())
    {
        hlog(HLOG_ERR, "tried grabbing the output from a process that hasn't completed yet");
        throw EProcessFutureNotFinished();
    }

    // lazy loading of the output string
    // check to see if the output string is empty
    // if so, and the output file wasn't the bit bucket
    // load it up and return it. Otherwise, we just load the
    // string we have already loaded
    if (mOutputString.empty() && mOutputFilename != "/dev/null") 
    {
        // read log file
        FString stmp;
        ifstream in(mOutputFilename, ios::in | ios::binary);
        char buf[4096];
  
        mOutputString.clear();
  
        while (in.good())
        {
            in.read(buf, sizeof(buf));
            stmp.assign(buf, in.gcount());
            mOutputString += stmp;
        }
        
        // cleanup
        in.close();
    } 
    else if(mOutputFilename == "/dev/null")
    {
        hlog(HLOG_WARN, "no output filename set");
        mOutputString.clear();
    }

    return mOutputString;
}

void Forte::ProcessFuture::Cancel()
{
    Signal(SIGTERM);
    Forte::Future<void>::Cancel();
}

void Forte::ProcessFuture::handlePDU(PDUPeer &peer)
{
    FTRACE;
    PDU pdu;
    while (peer.RecvPDU(pdu))
    {
        hlog(HLOG_DEBUG, "PDU opcode %d", pdu.opcode);
        switch (pdu.opcode)
        {
        case ProcessOpControlRes:
            handleControlRes(peer, pdu);
            break;
        case ProcessOpStatus:
            handleStatus(peer, pdu);
            break;
        default:
            hlog(HLOG_ERR, "unexpected PDU with opcode %d", pdu.opcode);
            break;
        }
    }
}

void Forte::ProcessFuture::handleControlRes(PDUPeer &peer, const PDU &pdu)
{
    FTRACE;
    const ProcessControlResPDU *resPDU = reinterpret_cast<const ProcessControlResPDU*>(pdu.payload);
    hlog(HLOG_DEBUG, "got back result code %d", resPDU->result);
    mMonitorPid = resPDU->monitorPID;
    mProcessPid = resPDU->processPID;
    mStatusCode = resPDU->result;
    if (resPDU->result == ProcessSuccess)
        setState(STATE_RUNNING);
    else
    {
        mErrorString.assign(resPDU->error);
        setState(STATE_ERROR);
    }
}

void Forte::ProcessFuture::handleStatus(PDUPeer &peer, const PDU &pdu)
{
    FTRACE;
    const ProcessStatusPDU *status = reinterpret_cast<const ProcessStatusPDU*>(pdu.payload);
    hlog(HLOG_DEBUG, "got back status type %d code %d", status->type, status->statusCode);
    mStatusCode = status->statusCode;
    switch (status->type)
    {
    case ProcessStatusStarted:
        setState(STATE_RUNNING);
        break;
    case ProcessStatusError:
        setState(STATE_ERROR);
        break;
    case ProcessStatusExited:
        setState(STATE_EXITED);
        break;
    case ProcessStatusKilled:
        setState(STATE_KILLED);
        break;
    case ProcessStatusStopped:
        setState(STATE_STOPPED);
        break;
    default:
        hlog(HLOG_ERR, "unknown status type!");
        break;
    }
}

void Forte::ProcessFuture::handleError(PDUPeer &peer)
{
    // the process monitor connection has encountered an unrecoverable
    // error.
    hlog(HLOG_ERR, "lost connection to procmon");
    if (!isInTerminalState())
    {
        mStatusCode = ProcessProcmonFailure;
        setState(STATE_ERROR);
    }
}
