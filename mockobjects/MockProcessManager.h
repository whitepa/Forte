#ifndef _MockProcessManager_h
#define _MockProcessManager_h

#include "ProcessManager.h"
#include "LogManager.h"
#include "PDUPeerSet.h"

namespace Forte
{
    EXCEPTION_CLASS(EMockProcessManager);
    EXCEPTION_SUBCLASS2(EMockProcessManager, EMockProcessManagerUnexpectedCommand,
                        "Unexpected command received!");
    EXCEPTION_SUBCLASS(EMockProcessManager, EMockProcessManagerUnableToOpenInputFile);
    EXCEPTION_SUBCLASS(EMockProcessManager, EMockProcessManagerUnableToOpenOutputFile);
    EXCEPTION_SUBCLASS(EMockProcessManager, EMockProcessManagerUnableToExec);
    EXCEPTION_SUBCLASS(EMockProcessManager, EMockProcessManagerUnknownMessage);
    
    struct ExpectedCommandResponse
    {
        FString mCommand;
        FString mResponse;
        int mResponseCode;
        int mCommandExecutionTime;  // in milliseconds
    };
    typedef boost::shared_ptr<ExpectedCommandResponse> ExpectedCommandResponsePtr; 

    class MockProcessHandler
    {
    public:
        MockProcessHandler(
            int fd,
            const ExpectedCommandResponsePtr& expectedResponse);
        virtual ~MockProcessHandler();

        virtual void Run(void);

    private:
        void pduCallback(PDUPeer &peer);
        void handleParam(const PDUPeer &peer, const PDU &pdu);
        void handleControlReq(const PDUPeer &peer, const PDU &pdu);
        void sendControlRes(const PDUPeer &peer, int resultCode, const char *desc = "");
        void mockStartProcess(void);
        void mockRunProcess(void);

        PDUPeerSet mPeerSet;
        ExpectedCommandResponsePtr mExpectedCommandResponse;

        FString mCmdline;
        FString mCWD;
        FString mInputFilename;
        FString mOutputFilename;
        bool mShutdown;
    };
    
    class MockProcessManager : public ProcessManager
    {
    public:
        MockProcessManager();

        virtual ~MockProcessManager();

        void SetCommandResponse(
            const FString& command,
            const FString& response,
            int responseCode = 0,
            int commandExecutionTime = 0
            );

        void ClearCommandResponse() {
            mCommandResponseMap.clear();
        };

    protected:
        virtual void startMonitor(boost::shared_ptr<Forte::ProcessFuture> ph);

        std::map<Forte::FString, ExpectedCommandResponsePtr> mCommandResponseMap;
        
    };
};
#endif
