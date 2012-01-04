#ifndef _GMockProcessManager_h
#define _GMockProcessManager_h

#include "ProcessManager.h"
#include <gmock/gmock.h>

namespace Forte
{
    class GMockProcessManager : public ProcessManager
    {
    public:
        GMockProcessManager() {}
        virtual ~GMockProcessManager() {}

        MOCK_METHOD6(CreateProcess, boost::shared_ptr<ProcessFuture> (
                         const FString &command,
                         const FString &currentWorkingDirectory,
                         const FString &outputFilename,
                         const FString &errorFilename,
                         const FString &inputFilename,
                         const StrStrMap *environment));

        MOCK_METHOD6(CreateProcessDontRun, boost::shared_ptr<ProcessFuture> (
                         const FString &command,
                         const FString &currentWorkingDirectory,
                         const FString &outputFilename,
                         const FString &errorFilename,
                         const FString &inputFilename,
                         const StrStrMap *environment));

        MOCK_METHOD5(CreateProcessAndGetResult, int (
                         const Forte::FString& command,
                         Forte::FString& output,
                         const Timespec &timeout,
                         const FString &inputFilename,
                         const StrStrMap *environment));

        MOCK_METHOD1(RunProcess, void (boost::shared_ptr<ProcessFuture> ph));

        MOCK_METHOD0(IsProcessMapEmpty, bool ());
    };

    typedef boost::shared_ptr<GMockProcessManager> GMockProcessManagerPtr;
};
#endif