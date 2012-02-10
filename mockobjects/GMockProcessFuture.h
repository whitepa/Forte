#ifndef _GMockProcessFuture_h
#define _GMockProcessFuture_h

#include "ProcessFuture.h"

namespace Forte
{
    class GMockProcessFuture : public ProcessFuture
    {
    public:
        MOCK_METHOD1(SetProcessCompleteCallback, 
                     void(ProcessFuture::ProcessCompleteCallback));
        MOCK_METHOD0(GetProcessCompleteCallback, 
                     ProcessFuture::ProcessCompleteCallback(void));
        MOCK_METHOD1(SetCurrentWorkingDirectory, void(const FString&));
        MOCK_METHOD1(SetEnvironment, void(const StrStrMap *));
        MOCK_METHOD1(SetInputFilename, void(const FString&));
        MOCK_METHOD1(SetOutputFilename, void(const FString&));
        MOCK_METHOD1(SetErrorFilename, void(const FString&));
        MOCK_METHOD0(GetResult, void());
        MOCK_METHOD1(GetResultTimed, void(const Timespec &));
        MOCK_METHOD1(Signal, void(int));
        MOCK_METHOD0(IsRunning, bool(void));
        MOCK_METHOD0(GetProcessPID, pid_t(void));
        MOCK_METHOD0(GetStatusCode, unsigned int(void));
        MOCK_METHOD0(GetProcessTerminationType, 
                     ProcessFuture::ProcessTerminationType(void));
        MOCK_METHOD0(GetOutputString, FString(void));
        MOCK_METHOD0(GetErrorString, FString(void));
        MOCK_METHOD0(Cancel, void(void));
        MOCK_METHOD0(GetMonitorPID, pid_t(void));
        MOCK_METHOD1(SetManagementChannel, void(boost::shared_ptr<PDUPeer> ));
        MOCK_METHOD0(GetCommand, FString(void));
    };
    typedef boost::shared_ptr<GMockProcessFuture> GMockProcessFuturePtr;
};

#endif
