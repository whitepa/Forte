#ifndef __ProcessHandle_h
#define __ProcessHandle_h

#include "Types.h"
#include "Object.h"
#include "ThreadCondition.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <sys/types.h>


namespace Forte
{

	enum ProcessTerminationType
	{
		ProcessExited,
		ProcessKilled,
		ProcessStopped,
		ProcessUnknownTermination
	};


    EXCEPTION_CLASS(EProcessHandle);
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleUnableToFork, "Unable to Fork Child Process");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleExecvFailed, "Execv Call Failed");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleProcessNotRunning, "Wait called on a non-running process");
	class ProcessManager;

    /**
     * A handle to a process
     *
     */
    class ProcessHandle : public Object 
    {
    public:
        typedef boost::function<void (boost::shared_ptr<ProcessHandle>)> ProcessCompleteCallback;
        
        
        /**
         * Construct a ProcessHandle object. Instantiating the object does not
         * automatically cause the command to run. You must call Run to kick off
         * the execution.
         * @param command The command you want to run
         * @param processCompleteCallback The callback to call when the process has completed
         * @param currentWorkingDirectory The directory that should be used as the current working
         * directory for the forked process.
         * @param environment a map of the environment to apply to the command
         * @param inputFilename name of the file to use for input.
         */
        ProcessHandle(const FString &command,
                      const FString &currentWorkingDirectory = "/",
                      const FString &inputFilename = "/dev/null",
                      const FString &outputFilename = "/dev/null", 
                      const StrStrMap *environment = NULL);
        virtual ~ProcessHandle();
        
        void SetProcessCompleteCallback(ProcessCompleteCallback processCompleteCallback);
		ProcessCompleteCallback GetProcessCompleteCallback() { return mProcessCompleteCallback; }
		
        void SetCurrentWorkingDirectory(const FString &cwd);
        void SetEnvironment(const StrStrMap *env);
        void SetInputFilename(const FString &infile);
		void SetProcessManager(ProcessManager *pm);
        
        pid_t Run();
        unsigned int Wait();
        void Cancel();
		void Abandon(bool signal = false);

        bool IsRunning();
		void SetIsRunning(bool running);
		
		FString GetGUID() { return mGUID; }
		
		pid_t GetChildPID() { return mChildPid; }
		
		void SetStatusCode(unsigned int code) { mStatusCode = code; }
        unsigned int GetStatusCode() { return mStatusCode; }
		
		void SetProcessTerminationType(ProcessTerminationType type) { mProcessTerminationType = type; }
		ProcessTerminationType GetProcessTerminationType() { return mProcessTerminationType; }
		
        FString GetOutputString();
        
    private:
        
        FString mCommand;
        ProcessCompleteCallback mProcessCompleteCallback;
        FString mCurrentWorkingDirectory;
        StrStrMap mEnvironment;
        FString mInputFilename;
        FString mOutputFilename;
		
		FString mGUID;
		pid_t mChildPid;
		unsigned int mStatusCode;
		ProcessTerminationType mProcessTerminationType;
		FString mOutputString;

        bool mIsRunning;
		Mutex mFinishedLock;
		ThreadCondition mFinishedCond;

        ProcessManager *mProcessManager;
        
        virtual FString shellEscape(const FString& arg);
    };


};
#endif
