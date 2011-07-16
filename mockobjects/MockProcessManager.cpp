
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/bind.hpp>
#include "DaemonUtil.h"
#include "MockProcessManager.h"
#include "ProcessFuture.h"
#include "LogManager.h"
#include "ProcessManagerPDU.h"
#include "FTrace.h"
#include "AutoFD.h"

using namespace Forte;

Forte::MockProcessHandler::MockProcessHandler(
    int fd,
    const ExpectedCommandResponsePtr& expectedResponse) :
    mExpectedCommandResponse (expectedResponse),
    mShutdown (false)
{
    mPeerSet.PeerCreate(fd);
}

Forte::MockProcessHandler::~MockProcessHandler()
{
}

void Forte::MockProcessHandler::Run()
{
    mPeerSet.SetupEPoll();
    mPeerSet.SetProcessPDUCallback(
        boost::bind(&MockProcessHandler::pduCallback, this, _1));

    while (mPeerSet.GetSize() > 0 && !mShutdown)
    {
        try
        {
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
            hlog(HLOG_ERR, "exception in Poll(): %s", e.what());
            // \TODO send error PDU
            throw;
        }     
    }
}

void Forte::MockProcessHandler::pduCallback(PDUPeer &peer)
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

void Forte::MockProcessHandler::handleParam(const PDUPeer &peer, const PDU &pdu)
{
    FTRACE;
    const ProcessParamPDU *paramPDU = reinterpret_cast<const ProcessParamPDU*>(pdu.payload);
    hlog(HLOG_DEBUG, "received param %d", paramPDU->param);
    switch (paramPDU->param)
    {
    case ProcessCmdline:
        hlog(HLOG_DEBUG, "Got command line param : %s", paramPDU->str);
        mCmdline.assign(paramPDU->str);
        break;
    case ProcessCwd:
        hlog(HLOG_DEBUG, "Got cwd param : %s", paramPDU->str);
        mCWD.assign(paramPDU->str);
        break;
    case ProcessInfile:
        hlog(HLOG_DEBUG, "Got infile param : %s", paramPDU->str);
        mInputFilename.assign(paramPDU->str);
        break;
    case ProcessOutfile:
        hlog(HLOG_DEBUG, "Got outfile param : %s", paramPDU->str);
        mOutputFilename.assign(paramPDU->str);
        break;
    default:
        hlog(HLOG_ERR, "received unknown param code %d", paramPDU->param);
        break;
    }
}

void Forte::MockProcessHandler::handleControlReq(const PDUPeer &peer, const PDU &pdu)
{
    FTRACE;
    const ProcessControlReqPDU *controlPDU = reinterpret_cast<const ProcessControlReqPDU*>(pdu.payload);

    try
    {
        switch (controlPDU->control)
        {
        case ProcessControlStart:
            mockStartProcess();
            hlog(HLOG_DEBUG, "Got a control request to start process");
            break;
        case ProcessControlSignal:
            hlog(HLOG_INFO, "Got a control request to signal process %d", controlPDU->signum);
            break;
        default:
            hlog(HLOG_ERR, "unknown control opcode %d", controlPDU->control);
            throw EMockProcessManagerUnknownMessage();
            break;
        }
        sendControlRes(peer, ProcessSuccess);

        // mock the actual running of process
        mockRunProcess();
    }
    catch (EMockProcessManagerUnableToOpenInputFile &e)
    {
        sendControlRes(peer, ProcessUnableToOpenInputFile);
        return;
    }
    catch (EMockProcessManagerUnableToOpenOutputFile &e)
    {
        sendControlRes(peer, ProcessUnableToOpenOutputFile);
        return;
    }
    catch (EMockProcessManagerUnableToExec &e)
    {
        sendControlRes(peer, ProcessUnableToExec);
        return;
    }
    catch (EMockProcessManager &e)
    {
        sendControlRes(peer, ProcessUnknownError, e.what());
        return;
    }

    // fake that the process exited with return code specified
    PDU p(ProcessOpStatus, sizeof(ProcessStatusPDU));
    ProcessStatusPDU *status = reinterpret_cast<ProcessStatusPDU*>(p.payload);
    gettimeofday(&status->timestamp, NULL);
    status->statusCode = mExpectedCommandResponse->mResponseCode;
    hlog(HLOG_DEBUG, "child exited (status %d)", status->statusCode);
    status->type = ProcessStatusExited;
    mPeerSet.SendAll(p);
    mShutdown = true;
}

void Forte::MockProcessHandler::sendControlRes(const PDUPeer &peer, int result, const char *desc)
{
    FTRACE;
    PDU p(ProcessOpControlRes, sizeof(ProcessControlResPDU));
    ProcessControlResPDU *response = reinterpret_cast<ProcessControlResPDU*>(p.payload);
    response->result = result;
    response->processPID = 1234;
    response->monitorPID = 5678;
    strncpy(response->error, desc, sizeof(response->error));
    peer.SendPDU(p);
}

void Forte::MockProcessHandler::mockStartProcess(void)
{
    FTRACE;

    // first check if the command line received 
    // is the same as the expected command line
    if (mCmdline != mExpectedCommandResponse->mCommand)
    {
        hlog(HLOG_DEBUG, "cmdline param is not same as expected %s != %s", 
             mCmdline.c_str(), mExpectedCommandResponse->mCommand.c_str());
        // this shouldn't happen but let's throw here
        throw EMockProcessManagerUnableToExec(
            "Command line param is not the same as the expected command");
    }

    // open the input and output files
    // these will throw if they are unable to open the files
    // and keep looping if they are interrupted during the open
    AutoFD outputfd;

    do
    {
        outputfd = open(mOutputFilename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        if(outputfd == -1 && errno != EINTR)
            throw EMockProcessManagerUnableToOpenOutputFile(FStringFC(), "%s", strerror(errno));
    }
    while (outputfd == -1 && errno == EINTR);

}

void Forte::MockProcessHandler::mockRunProcess(void)
{
    // open the input and output files
    // these will throw if they are unable to open the files
    // and keep looping if they are interrupted during the open
    AutoFD outputfd;

    do
    {
        outputfd = open(mOutputFilename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        if(outputfd == -1 && errno != EINTR)
            throw EMockProcessManagerUnableToOpenOutputFile(FStringFC(), "%s", strerror(errno));
    }
    while (outputfd == -1 && errno == EINTR);

    // now sleep for the expected amount of time
    if (mExpectedCommandResponse->mCommandExecutionTime > 0)
    {
        usleep(mExpectedCommandResponse->mCommandExecutionTime * 1000);
    }

    // write to the output file
    size_t sizeOfOutput = mExpectedCommandResponse->mResponse.size();
    if (sizeOfOutput > 0)
    {
        ssize_t wrote = 0;
        if ((wrote = write(outputfd, 
                           mExpectedCommandResponse->mResponse.c_str(),
                           sizeOfOutput)) == (ssize_t)sizeOfOutput)
        {
            // was able to write the whole response
            hlog(HLOG_DEBUG, "Wrote %zu of response for command %s", 
                 wrote, mExpectedCommandResponse->mResponse.c_str());
        }
        else
        {
            throw EMockProcessManagerUnableToExec();
        }
    }
}

Forte::MockProcessManager::MockProcessManager() :
    ProcessManager()
{
   
}

Forte::MockProcessManager::~MockProcessManager()
{
    deleting();
}

void Forte::MockProcessManager::SetCommandResponse(
    const FString& command,
    const FString& response,
    int responseCode,
    int commandExecutionTime)
{
    ExpectedCommandResponsePtr expectedResponse(
        new ExpectedCommandResponse());
    expectedResponse->mCommand = command;
    expectedResponse->mResponse = response;
    expectedResponse->mResponseCode = responseCode;
    expectedResponse->mCommandExecutionTime = commandExecutionTime;
    mCommandResponseMap[command] = expectedResponse;
}

void Forte::MockProcessManager::startMonitor(boost::shared_ptr<Forte::ProcessFuture> ph)
{

    // first try to see if we have this command in our map
    FString command = ph->GetCommand();
    ExpectedCommandResponsePtr expectedResponse;
    if (mCommandResponseMap.find(command) != 
        mCommandResponseMap.end())
    {
        // found it
        expectedResponse = mCommandResponseMap[command];
    }
    else
    {
        // didn't find it throw
        throw EMockProcessManagerUnexpectedCommand();
    }
    

    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
        throw EProcessManagerUnableToCreateSocket(FStringFC(), "%s", strerror(errno));
    AutoFD parentfd(fds[0]);
    AutoFD childfd(fds[1]);

    pid_t childPid = DaemonUtil::ForkSafely();
    if(childPid < 0) 
        throw EProcessManagerUnableToFork(FStringFC(), "%s", strerror(errno));
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

        MockProcessHandler handler(
            childfd, expectedResponse);
        handler.Run();
        exit(0);
    }
    else
    {
        // parent
        childfd.Close();
        // add a PDUPeer to the PeerSet owned by the ProcessManager
        //shared_ptr<ProcessManager> pm(mProcessManagerPtr.lock());
        ph->SetManagementChannel(addPeer(parentfd));
        parentfd.Release();
    }
}


int MockProcessManager::CreateProcessAndGetResult(const FString& command, 
                                                  FString& output, 
                                                  const int timeoutSeconds)
{
    ExpectedCommandResponsePtr expectedResponse;
    
    if (mCommandResponseMap.find(command) != mCommandResponseMap.end())
    {
        expectedResponse = mCommandResponseMap[command];
    }
    else
    {
        throw EMockProcessManagerUnexpectedCommand();
    }
   
    output = expectedResponse->mResponse;
    return expectedResponse->mResponseCode;
}

