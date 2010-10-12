// Process.cpp

#include "Process.h"
#include "ProcessManager.h"
#include "AutoMutex.h"
#include "Exception.h"
#include "LogManager.h"
#include "LogTimer.h"
#include "ServerMain.h"
#include "Util.h"
#include "FTrace.h"
#include "GUID.h"
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

Forte::Process::Process(const boost::shared_ptr<ProcessManager> &mgr,
                        const FString &procmon,
                        const FString &guid,
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
    mInputFD(-1),
    mOutputFD(-1),
    mGUID(guid),
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

Forte::Process::~Process() 
{
    try
    {
        if (mState == STATE_STARTING ||
            mState == STATE_RUNNING ||
            mState == STATE_STOPPED)
            Abandon();
    }
    catch (Exception &e)
    {
        hlog(HLOG_ERR, "%s", e.what().c_str());
    }
}

void Forte::Process::SetProcessCompleteCallback(ProcessCompleteCallback processCompleteCallback)
{
    if(mState != STATE_READY) 
    {
        hlog(HLOG_ERR, "tried setting the process complete callback after the process had been started");
        throw EProcessStarted();
    }
    mProcessCompleteCallback = processCompleteCallback;
}

void Forte::Process::SetCurrentWorkingDirectory(const FString &cwd) 
{
    if(mState != STATE_READY) 
    {
        hlog(HLOG_ERR, "tried setting the current working directory after the process had been started");
        throw EProcessStarted();
    }
    mCurrentWorkingDirectory = cwd;
}

void Forte::Process::SetEnvironment(const StrStrMap *env)
{
    if(mState != STATE_READY)
    {
        hlog(HLOG_ERR, "tried setting the environment after the process had been started");
        throw EProcessStarted();
    }
    if(env) 
    {
        mEnvironment.clear();
        mEnvironment.insert(env->begin(), env->end());
    }
}

void Forte::Process::SetInputFilename(const FString &infile)
{
    if(mState != STATE_READY)
    {
        hlog(HLOG_ERR, "tried setting the input filename after the process had been started");
        throw EProcessStarted();
    }
    mInputFilename = infile;
}

void Forte::Process::SetOutputFilename(const FString &outfile)
{
    if(mState != STATE_READY)
    {
        hlog(HLOG_ERR, "tried setting the output filename after the process had been started");
        throw EProcessStarted();
    }
    mOutputFilename = outfile;
}

boost::shared_ptr<ProcessManager> Forte::Process::GetProcessManager(void)
{
    shared_ptr<ProcessManager> mgr(mProcessManagerPtr.lock());
    if (!mgr)
    {
        throw EProcessHandleInvalid();
    }
    return mgr;
}

pid_t Forte::Process::Run()
{
    if (!mManagementChannel)
        throw EProcessHandleInvalid();

    if (mState != STATE_READY)
        throw EProcessStarted();

    // send the prepare PDU, with full command line info, etc
    PDU preparePDU(ProcessOpPrepare, sizeof(ProcessPreparePDU));
    ProcessPreparePDU *prepare = reinterpret_cast<ProcessPreparePDU*>(preparePDU.payload);

    // \TODO safe copy of these strings, ensure null termination,
    // disallow truncation (throw exception if the source strings are
    // too long)
    strncpy(prepare->cmdline, mCommand.c_str(), sizeof(prepare->cmdline));
    strncpy(prepare->cwd, mCurrentWorkingDirectory.c_str(), sizeof(prepare->cwd));

    mManagementChannel->SendPDU(preparePDU);

    // send the control PDU telling the process to start
    PDU pdu(ProcessOpControlReq, sizeof(ProcessControlReqPDU));
    ProcessControlReqPDU *control = reinterpret_cast<ProcessControlReqPDU*>(pdu.payload);
    control->control = ProcessControlStart;
    mManagementChannel->SendPDU(pdu);

    setState(STATE_STARTING);

    // wait for the process to change state
    AutoUnlockMutex lock(mWaitLock);
    while (mState == STATE_STARTING)
        mWaitCond.TimedWait(1);

    if (mState == STATE_ERROR)
        // \TODO throw specific error
        throw EProcess("Failed to start");
    else if (mState == STATE_RUNNING)
        hlog(HLOG_DEBUG, "process started");
    else
        throw EProcessUnknownState();

    return mProcessPid;
}

void Forte::Process::setState(int state)
{
    AutoUnlockMutex lock(mWaitLock);
    mState = state;
    mWaitCond.Broadcast();
}

void Forte::Process::startMonitor()
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
        throw EProcessUnableToCreateSocket(FStringFC(), "%s", strerror(errno));
    AutoFD parentfd(fds[0]);
    AutoFD childfd(fds[1]);

    pid_t childPid = fork();
    if(childPid < 0) 
        throw EProcessUnableToFork(FStringFC(), "%s", strerror(errno));
    else if(childPid == 0)
    {
        fprintf(stderr, "procmon child, childfd=%d\n", childfd.GetFD());
        // child
        // close all file descriptors, except the PDU channel
        while (close(parentfd) == -1 && errno == EINTR);
//        for (int n = 0; n < 1024; ++n)
//            if (n != childfd)
//                while (close(n) == -1 && errno == EINTR);
        // clear sig mask
        sigset_t set;
        sigemptyset(&set);
        pthread_sigmask(SIG_SETMASK, &set, NULL);

        // create a new process group / session
        setsid();

        char **vargs = new char* [3];
        vargs[0] = "(procmon)"; // \TODO include the name of the monitored process
        vargs[1] = const_cast<char *>(FString(childfd).c_str());
        vargs[2] = 0;
        fprintf(stderr, "procmon child, exec '%s' '%s'\n", mProcmonPath.c_str(), vargs[1]);
        execv(mProcmonPath, vargs);
        fprintf(stderr, "procmon child, exec failed: %d %s\n", errno, strerror(errno));
        // \TODO should we send back a message indicating the exact failure?
        exit(-1);
    }
    else
    {
        // parent
        close(childfd);
        // add a PDUPeer to the PeerSet owned by the ProcessManager
        shared_ptr<ProcessManager> pm(mProcessManagerPtr.lock());
        mManagementChannel = pm->addPeer(parentfd);
        parentfd.Release();
    }
}


//     sigset_t set;
//     mIsRunning = true;
//     char *argv[ARG_MAX];

//     // \TODO: need better handling of the number of args.  Currently
//     // if their are more than ARG_MAX arguments, we would crash.

//     // \TODO: ultimately we should be communicating back with the
//     // parent process via some sort of IPC, and notifying of any of
//     // these error details via that mechanism.  NOTE: Exceptions must
//     // not be thrown out of this method!  At this point we are running
//     // in a forked process, and throwing an exception (via calling
//     // exit() ) will cause us to end up with two copies of the process
//     // which originally called Run().

//     // we need to split the command up
//     std::vector<std::string> strings;
//     FString scrubbedCommand = shellEscape(mCommand);
//     boost::split(strings, scrubbedCommand, boost::is_any_of("\t "));
//     unsigned int num_args = strings.size();
//     for(unsigned int i = 0; i < num_args; ++i) 
//     {
//         // ugh - I hate this cast here, but as this process
//         // is about to be blown away, it is harmless, and
//         // much simpler than trying to avoid it
//         argv[i] = const_cast<char*>(strings[i].c_str());
//     }
//     argv[num_args] = 0;
		
//     // change current working directory
//     if (!mCurrentWorkingDirectory.empty()) 
//     {
//         if (chdir(mCurrentWorkingDirectory)) 
//         {
//             hlog(HLOG_CRIT, "Cannot change directory to: %s", mCurrentWorkingDirectory.c_str());
//             cerr << "Cannot change directory to: " << mCurrentWorkingDirectory << endl;
//             exit(-1);
//         }
//     }
		
//     // setup in, out, err
//     int dupResult = -1;
//     do
//     {
//         dupResult = dup2(mInputFD, 0);
//         if(dupResult == -1 && errno != EINTR)
//         {
//             hlog(HLOG_ERR, "unable to duplicate the input file descriptor (%d)", errno);
//             throw EProcessUnableToDuplicateInputFD();
//         }
//     }
//     while (dupResult == -1 && errno == EINTR);

//     dupResult = -1;
//     do
//     {
//         dupResult = dup2(mOutputFD, 1);
//         if(dupResult == -1 && errno != EINTR)
//         {
//             hlog(HLOG_ERR, "unable to duplicate the output file descriptor (%d)", errno);
//             throw EProcessUnableToDuplicateOutputFD();
//         }
//     }
//     while (dupResult == -1 && errno == EINTR);

//     dupResult = -1;
//     do
//     {
//         dupResult = dup2(mOutputFD, 2);
//         if(dupResult == -1 && errno != EINTR)
//         {
//             hlog(HLOG_ERR, "unable to duplicate the error file descriptor (%d)", errno);
//             throw EProcessUnableToDuplicateErrorFD();
//         }
//     }
//     while (dupResult == -1 && errno == EINTR);

//     // close all other FDs
//     int fdstart = 3;
//     int fdlimit = sysconf(_SC_OPEN_MAX);
//     while (fdstart < fdlimit) 
//     {
//         int closeResult = -1;
//         do
//         {
//             closeResult = close(fdstart);
//             if(closeResult == -1 && errno == EIO)
//             {
//                 hlog(HLOG_WARN, "unable to close parent file descriptor %d (EIO)", fdstart);
//             }
//         }
//         while(closeResult == -1 && errno == EINTR);

//         fdstart++;
            
//     }
		
//     // set up environment?
//     if (!mEnvironment.empty()) {
//         StrStrMap::const_iterator mi;
			
//         for (mi = mEnvironment.begin(); mi != mEnvironment.end(); ++mi) 
//         {
//             if (mi->second.empty()) unsetenv(mi->first);
//             else setenv(mi->first, mi->second, 1);
//         }
//     }
		
//     // clear sig mask
//     sigemptyset(&set);
//     pthread_sigmask(SIG_SETMASK, &set, NULL);
		
//     // create a new process group / session
//     setsid();

//     execv(argv[0], argv);
//     hlog(HLOG_ERR, "unable to execv the command");
//     throw EProcessExecvFailed();
// }

// void Forte::Process::RunParent(pid_t cpid)
// {
//     mChildPid = cpid;
//     // close the input and output fds we opened for our child
//     int closeResult = -1;
//     do
//     {
//         closeResult = close(mInputFD);
//         if(closeResult == -1 && errno != EINTR)
//         {
//             hlog(HLOG_ERR, "unable to close the child input file (%d)", errno);
//             throw EProcessUnableToCloseInputFile();
//         }
//         else 
//         {
//             mInputFD = -1;
//         }
//     }
//     while(closeResult == -1 && errno == EINTR);
    
//     closeResult = -1;
//     do
//     {
//         closeResult = close(mOutputFD);
//         if(closeResult == -1 && errno != EINTR)
//         {
//             hlog(HLOG_ERR, "unable to close the child output file (%d)", errno);
//             throw EProcessUnableToCloseOutputFile();
//         }
//         else
//         {
//             mOutputFD = -1;
//         }
//     }
//     while(closeResult == -1 && errno == EINTR);
    
//     // this just returns back to whence it was called
//     // the ProcessManager will now carry out the task
//     // of monitoring the running process.
//     mIsRunning = true;
// }

unsigned int Forte::Process::Wait()
{
    if(mState == STATE_READY) 
    {
        hlog(HLOG_ERR, "tried waiting on a process that has not been started");
        throw EProcessNotRunning();
    }

    if (mState == STATE_ABANDONED)
        throw EProcessAbandoned();

    hlog(HLOG_DEBUG, "waiting for process to end (%s)", mGUID.c_str());
    AutoUnlockMutex lock(mWaitLock);
    while (!isInTerminalState())
    {
        // \TODO need a wait which honors the current thread shutdown
        // status. Implement this via a static
        // Thread::interruptibleSleep() which can identify the correct
        // Thread object to call interruptibleSleep() on.  Then make a
        // ThreadCondition::InterruptibleWait() which works with that.
        mWaitCond.Wait();
    }
    return mStatusCode;
}

void Forte::Process::notifyWaiters()
{
    AutoUnlockMutex lock(mWaitLock);
    mWaitCond.Broadcast();
}

void Forte::Process::Abandon()
{
    if(!isInActiveState())
    {
        hlog(HLOG_ERR, "tried abandoning a process that is not currently running");
        throw EProcessNotRunning();
    }

    hlog(HLOG_DEBUG, "abandoning process %u (%s)", mProcessPid, mGUID.c_str());
    
    GetProcessManager()->abandonProcess(mManagementChannel->GetFD());

    // close the monitor fd
    if (mManagementChannel)
        while (close(mManagementChannel->GetFD()) == -1 && errno == EINTR);
    
    setState(STATE_ABANDONED);
}

void Forte::Process::Signal(int signum)
{
    if (!isInRunningState())
        throw EProcessNotRunning();

    PDU pdu(ProcessOpControlReq, sizeof(ProcessControlReqPDU));
    ProcessControlReqPDU *control = reinterpret_cast<ProcessControlReqPDU*>(pdu.payload);
    control->control = ProcessControlSignal;
    control->signum = signum;
    mManagementChannel->SendPDU(pdu);
}

bool Forte::Process::IsRunning()
{
    return isInRunningState();
}

unsigned int Forte::Process::GetStatusCode() 
{ 
    if(mState == STATE_READY)
    {
        hlog(HLOG_ERR, "tried grabbing the status code from a process that hasn't been started");
        throw EProcessNotStarted();
    }
    else if (!isInTerminalState())
    {
        hlog(HLOG_ERR, "tried grabbing the status code from a process that hasn't completed yet");
        throw EProcessNotFinished();
    }
    return mStatusCode;
}

Forte::Process::ProcessTerminationType Forte::Process::GetProcessTerminationType() 
{ 
    if(mState == STATE_READY)
    {
        hlog(HLOG_ERR, "tried grabbing the termination type from a process that hasn't been started");
        throw EProcessNotStarted();
    }
    else if (!isInTerminalState())
    {
        hlog(HLOG_ERR, "tried grabbing the termination type from a process that hasn't completed yet");
        throw EProcessNotFinished();
    }
    else if (mState == STATE_EXITED)
        return ProcessExited;
    else if (mState == STATE_KILLED)
        return ProcessKilled;
    else
        return ProcessUnknownTermination;
}

FString Forte::Process::GetOutputString()
{
    if(mState == STATE_READY)
    {
        hlog(HLOG_ERR, "tried grabbing the output from a process that hasn't been started");
        throw EProcessNotStarted();
    }
    else if(!isInTerminalState())
    {
        hlog(HLOG_ERR, "tried grabbing the output from a process that hasn't completed yet");
        throw EProcessNotFinished();
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

void Forte::Process::handlePDU(PDUPeer &peer)
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

void Forte::Process::handleControlRes(PDUPeer &peer, const PDU &pdu)
{
    FTRACE;
    const ProcessControlResPDU *resPDU = reinterpret_cast<const ProcessControlResPDU*>(pdu.payload);
    hlog(HLOG_DEBUG, "got back result code %d", resPDU->result);
    mMonitorPid = resPDU->monitorPID;
    mProcessPid = resPDU->processPID;
    if (resPDU->result == ProcessSuccess)
        setState(STATE_RUNNING);
    else
    {
        // \TODO set specific error code, allowing Run() to throw specific exception
        setState(STATE_ERROR);
    }
}

void Forte::Process::handleStatus(PDUPeer &peer, const PDU &pdu)
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
