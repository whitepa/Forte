#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "LogManager.h"
#include "ProcessMonitor.h"
#include "ProcessManagerPDU.h"
#include "FTrace.h"

bool Forte::ProcessMonitor::sGotSIGCHLD = false;

Forte::ProcessMonitor::ProcessMonitor(int argc, char *argv[]) :
    mInputFilename("/dev/null"),
    mOutputFilename("/dev/null")
{
    mLogManager.BeginLogging("/tmp/procmon.log"); // \TODO
    // single argument only, should be a file descriptor
    if (argc != 2)
        throw EProcessMonitorArguments();
    FString fdStr(argv[1]);
    if (!fdStr.IsUnsignedNumeric())
        throw EProcessMonitorArguments();
    int fd = fdStr.AsUnsignedInteger();
    mPeerSet.PeerCreate(fd);
    argv[1] = NULL;
}

Forte::ProcessMonitor::~ProcessMonitor()
{
}

void Forte::ProcessMonitor::Run()
{    
    // this is the "main function" of the management process.
    // install a SIGCHLD handler
    sGotSIGCHLD = false;
    sigset_t set;
    sigemptyset(&set);
    signal(SIGCHLD, handleSIGCHLD);
    signal(SIGPIPE, SIG_IGN);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    mPeerSet.SetupEPoll();

    mPeerSet.SetProcessPDUCallback(boost::bind(&ProcessMonitor::pduCallback, this, _1));

    // The process monitor should exit when:
    //  the process terminates AND all peer connections close
    while (isActiveState() || mPeerSet.GetSize() > 0)
    {
        try
        {
            if (sGotSIGCHLD)
                doWait();
            mPeerSet.Poll(500);
        }
        catch (EPDUPeerSetNoPeers &e)
        {
            // no problem, sleep and try again
            usleep(100000);
        }
        catch (EPDUPeerSetPollFailed &e)
        {
            // this is a fatal error, so we must send the error PDU
            hlog(HLOG_ERR, "exception in Poll(): %s", e.what().c_str());
            // \TODO send error PDU
            throw;
        }
    }
}

void Forte::ProcessMonitor::pduCallback(PDUPeer &peer)
{
    FTRACE;
    PDU pdu;
    while (peer.RecvPDU(pdu))
    {
        hlog(HLOG_DEBUG, "PDU opcode %d", pdu.opcode);
        switch(pdu.opcode)
        {
        case ProcessOpParam:
            handleParam(peer, pdu);
            break;
        case ProcessOpControlReq:
            handleControlReq(peer, pdu);
            break;
        default:
            hlog(HLOG_ERR, "unexpected PDU with opcode %d", pdu.opcode);
            break;
        }
    }
}

void Forte::ProcessMonitor::handleParam(const PDUPeer &peer, const PDU &pdu)
{
    // \TODO better string handling here, don't assume null
    // termination of paramPDU->str
    FTRACE;
    const ProcessParamPDU *paramPDU = reinterpret_cast<const ProcessParamPDU*>(pdu.payload);
    hlog(HLOG_DEBUG, "received param %d", paramPDU->param);
    switch (paramPDU->param)
    {
    case ProcessCmdline:
        mCmdline.assign(paramPDU->str);
        mState = STATE_READY;
        break;
    case ProcessCwd:
        mCWD.assign(paramPDU->str);
        break;
    case ProcessInfile:
        mInputFilename.assign(paramPDU->str);
        break;
    case ProcessOutfile:
        mOutputFilename.assign(paramPDU->str);
        break;
    default:
        hlog(HLOG_ERR, "received unknown param code %d", paramPDU->param);
        break;
    }
}
void Forte::ProcessMonitor::handleControlReq(const PDUPeer &peer, const PDU &pdu)
{
    FTRACE;
    const ProcessControlReqPDU *controlPDU = reinterpret_cast<const ProcessControlReqPDU*>(pdu.payload);
    try
    {
        switch (controlPDU->control)
        {
        case ProcessControlStart:
            startProcess();
            break;
        case ProcessControlSignal:
            signalProcess(controlPDU->signum);
            break;
        default:
            hlog(HLOG_ERR, "unknown control opcode %d", controlPDU->control);
            throw EProcessMonitorUnknownMessage();
            break;
        }
        sendControlRes(peer, ProcessSuccess);
    }
    catch (EProcessMonitorUnableToOpenInputFile &e)
    {
        sendControlRes(peer, ProcessUnableToOpenInputFile);
    }
    catch (EProcessMonitorUnableToOpenOutputFile &e)
    {
        sendControlRes(peer, ProcessUnableToOpenOutputFile);
    }
    catch (EProcessMonitor &e)
    {
        // \TODO catch specific errors and send specific error codes
        sendControlRes(peer, ProcessUnknownError, e.what().c_str());
    }
}

void Forte::ProcessMonitor::sendControlRes(const PDUPeer &peer, int result, const char *desc)
{
    FTRACE;
    PDU p(ProcessOpControlRes, sizeof(ProcessControlResPDU));
    ProcessControlResPDU *response = reinterpret_cast<ProcessControlResPDU*>(p.payload);
    response->result = result;
    response->processPID = mPID;
    response->monitorPID = getpid();
    strncpy(response->error, desc, sizeof(response->error));
    peer.SendPDU(p);
}

void Forte::ProcessMonitor::handleSIGCHLD(int sig)
{
    sGotSIGCHLD = true;
}

void Forte::ProcessMonitor::doWait(void)
{
    FTRACE;
    sGotSIGCHLD = false;
    int child_status;
    pid_t tpid = waitpid(-1, &child_status, WNOHANG);
    if(tpid == -1)
    {
        if(errno == ECHILD)
        {
            hlog(HLOG_WARN, "wait() error: no unwaited-for children");
        }
        else if(errno == EINTR)
        {
            hlog(HLOG_ERR, "wait() error: unblocked signal or SIGCHILD caught");
        }
        else if(errno == EINVAL)
        {
            hlog(HLOG_CRIT, "wait() error: invalid options parameter");
        }
        else
        {
            hlog(HLOG_ERR, "wait() error: unknown error");
        }
    }
    else if (tpid == mPID)
    {
        PDU p(ProcessOpStatus, sizeof(ProcessStatusPDU));
        ProcessStatusPDU *status = reinterpret_cast<ProcessStatusPDU*>(p.payload);
        gettimeofday(&status->timestamp, NULL);
        if (WIFEXITED(child_status))
        {
            status->statusCode = WEXITSTATUS(child_status);
            hlog(HLOG_DEBUG, "child exited (status %d)", status->statusCode);
            status->type = ProcessStatusExited;
            mState = STATE_EXITED;
        }
        else if (WIFSIGNALED(child_status))
        {
            status->statusCode = WTERMSIG(child_status);
            status->type = ProcessStatusKilled;
            hlog(HLOG_DEBUG, "child killed (signal %d)", status->statusCode);
            mState = STATE_EXITED;
        }
        else if (WIFSTOPPED(child_status))
        {
            status->statusCode = WSTOPSIG(child_status);
            status->type = ProcessStatusStopped;
            hlog(HLOG_DEBUG, "child stopped (signal %d)", status->statusCode);
            mState = STATE_STOPPED;
        }
        else if (WIFCONTINUED(child_status))
        {
            status->statusCode = 0;
            status->type = ProcessStatusContinued;
            hlog(HLOG_DEBUG, "child continued");
            mState = STATE_RUNNING;
        }
        else
        {
            status->statusCode = child_status;
            status->type = ProcessStatusUnknownTermination;
            hlog(HLOG_ERR, "unknown child exit status (0x%x)", child_status);
            mState = STATE_EXITED;
        }
        // Send the status PDU to all peer connections
        mPeerSet.SendAll(p);
    }
    else
    {
        hlog(HLOG_ERR, "wait() returned a process id we don't know about (%u)", tpid);
    }
}

void Forte::ProcessMonitor::startProcess(void)
{
    FTRACE;
    if (mState != STATE_READY)
        throw EProcessMonitor(); // \TODO specific errors

    // open the input and output files
    // these will throw if they are unable to open the files
    // and keep looping if they are interrupted during the open
    int inputfd, outputfd;
    do
    {
        inputfd = open(mInputFilename, O_RDWR);
        if(inputfd == -1 && errno != EINTR)
            throw EProcessMonitorUnableToOpenInputFile(FStringFC(), "%s", strerror(errno));
    } 
    while (inputfd == -1 && errno == EINTR);

    do
    {
        outputfd = open(mOutputFilename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        if(outputfd == -1 && errno != EINTR)
            throw EProcessMonitorUnableToOpenOutputFile(FStringFC(), "%s", strerror(errno));
    }
    while (outputfd == -1 && errno == EINTR);

    pid_t pid;
    if ((pid = fork()) == 0)
    {
        // child
        sigset_t set;
        char *argv[ARG_MAX];

        // we need to split the command up

        // \TODO we should probably run it as /bin/bash -c cmdline
        // until we have a complete command line parser
        std::vector<std::string> strings;
        FString scrubbedCommand = shellEscape(mCmdline);
        boost::split(strings, scrubbedCommand, boost::is_any_of("\t "));
        unsigned int num_args = strings.size();
        for(unsigned int i = 0; i < num_args; ++i) 
        {
            // ugh - I hate this cast here, but as this process
            // is about to be blown away, it is harmless, and
            // much simpler than trying to avoid it
            argv[i] = const_cast<char*>(strings[i].c_str());
        }
        argv[num_args] = 0;
	
        // change current working directory
        if (!mCWD.empty() && chdir(mCWD))
        {
            hlog(HLOG_CRIT, "Cannot change directory to: %s", mCWD.c_str());
            exit(-1);
        }

        while (dup2(inputfd, 0) == -1 && errno == EINTR);
        while (dup2(outputfd, 1) == -1 && errno == EINTR);
        while (dup2(outputfd, 2) == -1 && errno == EINTR); // \TODO stderr?

        // close all other FDs
        int fdstart = 3;
        int fdlimit = sysconf(_SC_OPEN_MAX);
        while (fdstart < fdlimit) 
        {
            int closeResult = -1;
            do
            {
                closeResult = close(fdstart);
                if(closeResult == -1 && errno == EIO)
                {
                    hlog(HLOG_WARN, "unable to close parent file descriptor %d (EIO)", fdstart);
                }
            }
            while(closeResult == -1 && errno == EINTR);
            ++fdstart;
        }
//     // set up environment? \TODO do we need this?
//     if (!mEnvironment.empty()) {
//         StrStrMap::const_iterator mi;
			
//         for (mi = mEnvironment.begin(); mi != mEnvironment.end(); ++mi) 
//         {
//             if (mi->second.empty()) unsetenv(mi->first);
//             else setenv(mi->first, mi->second, 1);
//         }
//     }
        // clear sig mask
        sigemptyset(&set);
        pthread_sigmask(SIG_SETMASK, &set, NULL);
        
        // create a new process group / session
        setsid();
        
        execv(argv[0], argv);
        hlog(HLOG_ERR, "unable to execv the command");
        exit(-1);
    }
    // parent
    while (close(inputfd) == -1 && errno == EINTR);
    while (close(outputfd) == -1 && errno == EINTR);

    if (pid < 0)
    {
        // failed to fork
    }
    else
    {
        // successful
        mPID = pid;
        mState = STATE_RUNNING;
    }
}

void Forte::ProcessMonitor::signalProcess(int signal)
{
    FTRACE;
    hlog(HLOG_DEBUG, "state = %d", mState);
    if ((mState == STATE_RUNNING ||
         mState != STATE_STOPPED) &&
        mPID != 0 &&
        kill(mPID, signal) == -1)
        throw EProcessSignalFailed();
}

Forte::FString Forte::ProcessMonitor::shellEscape(const FString& arg) 
{
    // \TODO: implement this.
    hlog(HLOG_WARN, "NOT IMPLEMENTED!");
    return arg;
}
