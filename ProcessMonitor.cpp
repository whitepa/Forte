#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <boost/bind.hpp>
#include "DaemonUtil.h"
#include "LogManager.h"
#include "ProcessMonitor.h"
#include "ProcessManagerPDU.h"
#include "PDUPeerSetBuilderImpl.h"
#include "FTrace.h"
#include "PDU.h"
#include "SystemCallUtil.h"

#pragma GCC diagnostic ignored "-Wold-style-cast"

bool Forte::ProcessMonitor::sGotSIGCHLD = false;

Forte::ProcessMonitor::ProcessMonitor(int argc, char *argv[]) :
    mPeerSet(new PDUPeerSetBuilderImpl()),
    mInputFilename("/dev/null"),
    mOutputFilename("/dev/null"),
    mErrorFilename("/dev/null")
{
    FTRACE;
    FString logFile;
    FString logLevel;
    try
    {
        mServiceConfig.ReadConfigFile("/etc/procmon.conf");
        logFile = mServiceConfig.Get("logfile");
        logLevel = mServiceConfig.Get("loglevel");
    }
    catch (EServiceConfig &e)
    {
        // failed to load config, use some defaults
    }
    if (logFile != "")
    {
        mLogManager.SetGlobalLogMask(
            mLogManager.ComputeLogMaskFromString(logLevel));
        mLogManager.BeginLogging(logFile);
    }

    // single argument only, should be a file descriptor
    if (argc != 2)
        throw EProcessMonitorArguments();
    FString fdStr(argv[1]);
    if (!fdStr.IsUnsignedNumeric())
        throw EProcessMonitorArguments();
    int fd = fdStr.AsUnsignedInteger();
    mPeerSet->PeerCreate(fd);
    argv[1] = NULL;
}

Forte::ProcessMonitor::~ProcessMonitor()
{
    FTRACE;
}

void Forte::ProcessMonitor::Run()
{
    FTRACE;

    // this is the "main function" of the management process.
    // install a SIGCHLD handler
    sGotSIGCHLD = false;
    sigset_t set;
    sigemptyset(&set);
    signal(SIGCHLD, handleSIGCHLD);
    signal(SIGPIPE, SIG_IGN);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    mPeerSet->SetEventCallback(
        boost::bind(&ProcessMonitor::pduCallback, this, _1));
    mPeerSet->StartPolling();

    // The process monitor should exit when:
    //  the process terminates AND all peer connections close
    while (isActiveState() || mPeerSet->GetConnectedCount() > 0)
    {
        try
        {
            if (sGotSIGCHLD)
                doWait();

            usleep(500);
        }
        catch (EPDUPeerSetNoPeers &e)
        {
            // no problem, sleep and try again
            usleep(100000);
        }
        catch (EPDUPeerSetPollFailed &e)
        {
            // this is a fatal error, so we must send the error PDU
            hlog(HLOG_ERR, "exception in Poll(): %s", e.what());
            // \TODO send error PDU
            throw;
        }
    }
}

void Forte::ProcessMonitor::pduCallback(PDUPeerEventPtr event)
{
    FTRACE;
    if (event->mEventType == PDUPeerReceivedPDUEvent)
    {
        PDU pdu;
        while (event->mPeer->RecvPDU(pdu))
        {
            hlog(HLOG_DEBUG, "PDU opcode %d", pdu.GetOpcode());
            switch(pdu.GetOpcode())
            {
            case ProcessOpParam:
                handleParam(*(event->mPeer), pdu);
                break;
            case ProcessOpControlReq:
                handleControlReq(*(event->mPeer), pdu);
                break;
            default:
                hlog(HLOG_ERR, "unexpected PDU with opcode %d", pdu.GetOpcode());
                break;
            }
        }
    }
}

void Forte::ProcessMonitor::handleParam(const PDUPeer &peer, const PDU &pdu)
{
    // \TODO better string handling here, don't assume null
    // termination of paramPDU->str
    FTRACE;
    const ProcessParamPDU *paramPDU = pdu.GetPayload<ProcessParamPDU>();
    hlog(HLOG_DEBUG, "received param %d", paramPDU->param);
    switch (paramPDU->param)
    {
    case ProcessCmdline:
        mCmdline.assign(paramPDU->str);
        mState = STATE_READY;
        break;
    case ProcessCmdlineToLog:
        mCmdlineToLog.assign(paramPDU->str);
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
    case ProcessErrfile:
        mErrorFilename.assign(paramPDU->str);
        break;
    default:
        hlog(HLOG_ERR, "received unknown param code %d", paramPDU->param);
        break;
    }
}
void Forte::ProcessMonitor::handleControlReq(PDUPeer &peer, const PDU &pdu)
{
    FTRACE;
    const ProcessControlReqPDU *controlPDU =
        pdu.GetPayload<ProcessControlReqPDU>();
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
    catch (EProcessMonitorUnableToCWD &e)
    {
        sendControlRes(peer, ProcessUnableToCWD);
    }
    catch (EProcessMonitorUnableToFork &e)
    {
        sendControlRes(peer, ProcessUnableToFork);
    }
    catch (EProcessMonitorUnableToExec &e)
    {
        sendControlRes(peer, ProcessUnableToExec);
    }
    catch (EProcessMonitor &e)
    {
        sendControlRes(peer, ProcessUnknownError, e.what());
    }
}

void Forte::ProcessMonitor::sendControlRes(PDUPeer &peer, int result, const char *desc)
{
    FTRACE;
    PDU p(ProcessOpControlRes, sizeof(ProcessControlResPDU));
    ProcessControlResPDU *response = p.GetPayload<ProcessControlResPDU>();
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
        ProcessStatusPDU *status = p.GetPayload<ProcessStatusPDU>();
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
        if (status->statusCode != 0)
        {
            if (mCmdlineToLog.empty())
            {
                hlog(HLOG_INFO, "[%s] exit code is %i", mCmdline.c_str(),
                     status->statusCode);
            }
            else
            {
                hlog(HLOG_INFO, "[%s] exit code is %i", mCmdlineToLog.c_str(),
                     status->statusCode);
            }
        }
        // Send the status PDU to all peer connections
        mPeerSet->Broadcast(p);
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
        throw EProcessMonitorInvalidState();

    // open the input and output files
    // these will throw if they are unable to open the files
    // and keep looping if they are interrupted during the open
    int inputfd, outputfd, errorfd;
    do
    {
        inputfd = open(mInputFilename, O_RDWR);
        if(inputfd == -1 && errno != EINTR)
            throw EProcessMonitorUnableToOpenInputFile(
                SystemCallUtil::GetErrorDescription(errno));
    }
    while (inputfd == -1 && errno == EINTR);

    do
    {
        outputfd = open(mOutputFilename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        if(outputfd == -1 && errno != EINTR)
            throw EProcessMonitorUnableToOpenOutputFile(
                SystemCallUtil::GetErrorDescription(errno));
    }
    while (outputfd == -1 && errno == EINTR);

    do
    {
        errorfd = open(mErrorFilename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        if(errorfd == -1 && errno != EINTR)
            throw EProcessMonitorUnableToOpenErrorFile(
                SystemCallUtil::GetErrorDescription(errno));
    }
    while (errorfd == -1 && errno == EINTR);

    if (mCmdlineToLog.empty())
    {
        hlog(HLOG_INFO, "running command: %s", mCmdline.c_str());
    }
    else
    {
        hlog(HLOG_INFO, "running command: %s", mCmdlineToLog.c_str());
    }

    pid_t pid;
    if ((pid = DaemonUtil::ForkSafely()) == 0)
    {
        // child
        sigset_t set;
        char *argv[128]; // @TODO - don't hardcode a max # of args
        size_t cmdEnvLen = mCmdline.length() + 15;
        unsigned int envCount = 0;
        while (environ[envCount] != 0)
        {
            cmdEnvLen += strlen(environ[envCount]);
            envCount++;
        }
        hlog(HLOG_DEBUG, "Total command size: %u", (unsigned)cmdEnvLen);
        if (cmdEnvLen >= ARG_MAX)
            throw EProcessMonitorArgumentsTooLong();

        // we need to split the command up

        // \TODO we should probably run it as /bin/bash -c cmdline
        // until we have a complete command line parser
        std::vector<std::string> strings;
        strings.push_back("/bin/bash");
        strings.push_back("-c");
        strings.push_back(mCmdline);

        unsigned int num_args = strings.size();
        for(unsigned int i = 0; i < num_args; ++i)
        {
            // ugh - I hate this cast here, but as this process
            // is about to be blown away, it is harmless, and
            // much simpler than trying to avoid it
            // @TODO - fix this
            argv[i] = const_cast<char*>(strings[i].c_str());
        }
        argv[num_args] = 0;

        // change current working directory
        if (!mCWD.empty() && chdir(mCWD))
        {
            hlog(HLOG_CRIT, "Cannot change directory to: %s", mCWD.c_str());
            throw EProcessMonitorUnableToCWD();
        }

        while (dup2(inputfd, 0) == -1 && errno == EINTR);
        while (dup2(outputfd, 1) == -1 && errno == EINTR);
        while (dup2(errorfd, 2) == -1 && errno == EINTR);

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
        throw EProcessMonitorUnableToExec();
    }
    // parent
    while (close(inputfd) == -1 && errno == EINTR);
    while (close(outputfd) == -1 && errno == EINTR);
    while (close(errorfd) == -1 && errno == EINTR);

    if (pid < 0)
    {
        // failed to fork
        throw EProcessMonitorUnableToFork();
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
        throw EProcessFutureSignalFailed();
}

Forte::FString Forte::ProcessMonitor::shellEscape(const FString& arg)
{
    // \TODO: implement this.
    hlog(HLOG_WARN, "NOT IMPLEMENTED!");
    return arg;
}
