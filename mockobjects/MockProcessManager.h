#ifndef _MockProcessManager_h
#define _MockProcessManager_h

#include "ProcessManager.h"
#include "LogManager.h"
#include "FileSystem.h"
#include <queue>

namespace Forte
{
    EXCEPTION_CLASS(EMockProcessManager);
    EXCEPTION_SUBCLASS2(EMockProcessManager, EMockProcessManagerUnexpectedCommand,
                        "Unexpected command received!");
    EXCEPTION_SUBCLASS(EMockProcessManager, EMockProcessManagerUnableToOpenInputFile);
    EXCEPTION_SUBCLASS(EMockProcessManager, EMockProcessManagerUnableToOpenOutputFile);
    EXCEPTION_SUBCLASS(EMockProcessManager, EMockProcessManagerUnableToOpenErrorFile);
    EXCEPTION_SUBCLASS(EMockProcessManager, EMockProcessManagerUnableToExec);
    EXCEPTION_SUBCLASS(EMockProcessManager, EMockProcessManagerUnknownMessage);

    struct ExpectedCommandResponse
    {
        FString mCommand;
        FString mResponse;
        FString mErrorResponse;
        int mResponseCode;
        Timespec mDuration;
    };
    typedef boost::shared_ptr<ExpectedCommandResponse> ExpectedCommandResponsePtr;

    class MockProcessManagerThread : public Thread
    {
    public:
        MockProcessManagerThread() { initialized(); }
        ~MockProcessManagerThread() { deleting(); }
    protected:
        virtual void * run(void);
    };

    class MockProcessManager : public ProcessManager
    {
    public:
        MockProcessManager(const boost::shared_ptr<Forte::FileSystem> &fs);

        virtual ~MockProcessManager();

        void SetCommandResponse(
            const FString& command,
            const FString& response,
            const FString& errorResponse = "",
            int responseCode = 0,
            int commandExecutionTimeMillisec = 0
            );
        /*
         * Queue a command to run.  Queued commands take priority over
         * those in the map of expected commands
         */
        void QueueCommandResponse(
            const FString& command,
            const FString& response,
            int responseCode = 0,
            int commandExecutionTime = 0
            );
        unsigned int GetCommandInvocationCount(const FString& command);
        void ClearCommandResponse() {
            mCommandResponseMap.clear();
        };
        void ClearQueuedCommandResponse() {
            while (!mCommandResponseQueue.empty())
            {
                mCommandResponseQueue.pop();
            }
        }
        bool IsCommandQueueEmpty() {
            return (mCommandResponseQueue.empty());
        }

        bool HadUnexpectedCommand() {
            return mHadUnexpectedCommand;
        }

        virtual boost::shared_ptr<ProcessFuture> CreateProcess(
            const FString &command,
            const FString &currentWorkingDirectory = "/",
            const FString &outputFilename = "/dev/null",
            const FString &errorFilename = "/dev/null",
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL,
            const FString &commandToLog = "");

        virtual boost::shared_ptr<ProcessFuture> CreateProcessDontRun(
            const FString &command,
            const FString &currentWorkingDirectory = "/",
            const FString &outputFilename = "/dev/null",
            const FString &errorFilename = "/dev/null",
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL,
            const FString &commandToLog = "");

        virtual int CreateProcessAndGetResult(
            const Forte::FString& command,
            Forte::FString& output,
            Forte::FString& errorOutput,
            bool throwErrorOnNonZeroReturnCode = true,
            const Timespec &timeout = Timespec::FromSeconds(-1),
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL,
            const FString &commandToLog = "");

        virtual void RunProcess(boost::shared_ptr<ProcessFuture> ph);

        virtual bool IsProcessMapEmpty(void) { throw EUnimplemented(); }

        virtual std::list<Forte::FString> GetCommandInvocationList(void) {
            return mCommandInvocationList;
        }

    protected:
        ExpectedCommandResponsePtr lookupExpectedResponse(const FString &command);

        /**
         * Lock for the command list map
         */
        Mutex mCommandListLock;
        std::map<Forte::FString, ExpectedCommandResponsePtr> mCommandResponseMap;
        std::queue<ExpectedCommandResponsePtr> mCommandResponseQueue;
        std::map<Forte::FString, unsigned int> mCommandInvocationCount; // number of times this command/response was invoked
        std::list<Forte::FString> mCommandInvocationList;
        bool mHadUnexpectedCommand;

        boost::shared_ptr<Forte::FileSystem> mFS;
    };

    typedef boost::shared_ptr<MockProcessManager> MockProcessManagerPtr;
};
#endif
