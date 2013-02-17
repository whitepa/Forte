
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/bind.hpp>
#include "DaemonUtil.h"
#include "MockProcessManager.h"
#include "MockProcessFuture.h"
#include "LogManager.h"
#include "FTrace.h"
#include "AutoFD.h"
#include <boost/make_shared.hpp>

using namespace Forte;

void * Forte::MockProcessManagerThread::run(void)
{
    while (!Thread::IsShuttingDown())
    {
        interruptibleSleep(Timespec::FromSeconds(1));
    }
    return NULL;
}

Forte::MockProcessManager::MockProcessManager(const boost::shared_ptr<Forte::FileSystem> &fs) :
    mHadUnexpectedCommand(false),
    mFS(fs)
{
    FTRACE;
}

Forte::MockProcessManager::~MockProcessManager()
{
    FTRACE;
}

void Forte::MockProcessManager::SetCommandResponse(
    const FString& command,
    const FString& response,
    const FString& errorResponse,
    int responseCode,
    int commandExecutionTime)
{
    FTRACE2("%s", command.c_str());

    AutoUnlockMutex lock(mCommandListLock);
    ExpectedCommandResponsePtr expectedResponse(
        new ExpectedCommandResponse());
    expectedResponse->mCommand = command;
    expectedResponse->mResponse = response;
    expectedResponse->mErrorResponse = errorResponse;
    expectedResponse->mResponseCode = responseCode;
    expectedResponse->mDuration = Forte::Timespec::FromMillisec(commandExecutionTime);
    mCommandResponseMap[command] = expectedResponse;
    mCommandInvocationCount[command] = 0;
}

void MockProcessManager::QueueCommandResponse(const FString& command,
                                              const FString& response,
                                              int responseCode,
                                              int commandExecutionTime)
{
    FTRACE2("%s, %s", command.c_str(), response.c_str());

    AutoUnlockMutex lock(mCommandListLock);
    ExpectedCommandResponsePtr expectedResponse = boost::make_shared<ExpectedCommandResponse>();
    expectedResponse->mCommand = command;
    expectedResponse->mResponse = response;
    expectedResponse->mResponseCode = responseCode;
    expectedResponse->mDuration = Forte::Timespec::FromMillisec(commandExecutionTime);
    mCommandResponseQueue.push(expectedResponse);
    mCommandInvocationCount[command] = 0;    
}

unsigned int MockProcessManager::GetCommandInvocationCount(const FString& command)
{
    FTRACE2("%s", command.c_str());

    AutoUnlockMutex lock(mCommandListLock);
    std::map<FString, unsigned int>::const_iterator it;
    if ((it = mCommandInvocationCount.find(command)) != mCommandInvocationCount.end())
    {
        return (*it).second;
    }

    return 0;
}

boost::shared_ptr<ProcessFuture> MockProcessManager::CreateProcess(
    const FString &command,
    const FString &currentWorkingDirectory,
    const FString &outputFilename,
    const FString &errorFilename,
    const FString &inputFilename,
    const StrStrMap *environment,
    const FString &commandToLog)
{
    FTRACE2("%s", command.c_str());

    // look up the output string
    ExpectedCommandResponsePtr expectedResponse = lookupExpectedResponse(command);
    
    boost::shared_ptr<MockProcessFuture> pf(
        new MockProcessFuture(command,
                              outputFilename,
                              inputFilename,
                              expectedResponse,
                              mFS));
    mCommandInvocationCount[command]++;
    mCommandInvocationList.push_back(command);
    pf->Start();
    return pf;
}

boost::shared_ptr<ProcessFuture> MockProcessManager::CreateProcessDontRun(
    const FString &command,
    const FString &currentWorkingDirectory,
    const FString &outputFilename,
    const FString &errorFilename,
    const FString &inputFilename,
    const StrStrMap *environment,
    const FString &commandToLog)
{
    FTRACE2("%s", command.c_str());
    throw EUnimplemented();
    return boost::shared_ptr<ProcessFuture>();    
}

int MockProcessManager::CreateProcessAndGetResult(
    const FString& command,
    FString& output,
    FString& errorOutput,
    bool throwErrorOnNonZeroReturnCode,
    const Timespec &timeout,
    const FString &inputFileName,
    const StrStrMap *environment,
    const FString &commandToLog)
{
    FTRACE2("%s", command.c_str());
    ExpectedCommandResponsePtr expectedResponse = lookupExpectedResponse(command);

    output = expectedResponse->mResponse;
    mCommandInvocationCount[command]++;
    mCommandInvocationList.push_back(command);
    hlog(HLOG_DEBUG, "Returning response code %i",
         expectedResponse->mResponseCode);
    return expectedResponse->mResponseCode;
}

void MockProcessManager::RunProcess(boost::shared_ptr<ProcessFuture> ph)
{
    boost::shared_ptr<MockProcessFuture> mpf = 
        boost::dynamic_pointer_cast<MockProcessFuture>(ph);
    if (mpf->HasStarted())
        throw_exception(EProcessFutureStarted());
    mpf->Start();
}

ExpectedCommandResponsePtr MockProcessManager::lookupExpectedResponse(const FString &command)
{
    AutoUnlockMutex lock(mCommandListLock);
    ExpectedCommandResponsePtr expectedResponse;
    if (!mCommandResponseQueue.empty())
    {
        expectedResponse = mCommandResponseQueue.front();
        if (expectedResponse->mCommand != command)
        {
            hlog(HLOG_ERR, "Front of queue is '%s'; expected '%s'",
                 expectedResponse->mCommand.c_str(), command.c_str());
            throw_exception(EMockProcessManagerUnexpectedCommand(command));
        }
        mCommandResponseQueue.pop();
    }
    else if (mCommandResponseMap.find(command) != mCommandResponseMap.end())
    {
        hlog(HLOG_DEBUG2, "Found command '%s'", command.c_str());
        expectedResponse = mCommandResponseMap[command];
    }
    else
    {
        mHadUnexpectedCommand=true;
        hlog_and_throw(HLOG_ERR, EMockProcessManagerUnexpectedCommand(command));
    }
    return expectedResponse;
}
