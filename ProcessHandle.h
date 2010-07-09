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
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleUnableToOpenInputFile, "Unable to open the input file");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleUnableToCloseInputFile, "Unable to close the input file");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleUnableToOpenOutputFile, "Unable to open the output file");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleUnableToCloseOutputFile, "Unable to close the output file");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleUnableToFork, "Unable to Fork Child Process");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleExecvFailed, "Execv Call Failed");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleProcessNotRunning, "Wait called on a non-running process");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleProcessRunning, "method called on a running process");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleProcessStarted, "method called on a process that has already started");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleProcessNotFinished, "method called on a process that is not finished yet");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleUnableToDuplicateInputFD, "Unable to duplicate Input File Descriptor");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleUnableToDuplicateOutputFD, "Unable to duplicate Output File Descriptor");
    EXCEPTION_SUBCLASS2(EProcessHandle, EProcessHandleUnableToDuplicateErrorFD, "Unable to duplicate Error File Descriptor");


	class ProcessManager;

    /**
     * A handle to a process managed by ProcessManager
     */
    class ProcessHandle : public Object 
    {
    public:
        typedef boost::function<void (boost::shared_ptr<ProcessHandle>)> ProcessCompleteCallback;
        
        
        /**
         * Construct a ProcessHandle object. Instantiating the object does not
         * automatically cause the command to run. You must call Run to kick off
         * the execution.
         *
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

        /**
         * destructor for a ProcessHandle.
         */
        virtual ~ProcessHandle();
        
        /**
         * set the callback function that should be executed when the process is complete.
         *
         * @param processCompleteCallback a ProcessCompleteCallback function object
         */
        void SetProcessCompleteCallback(ProcessCompleteCallback processCompleteCallback);

        /**
         * GetProcessCompleteCallback() returns the callback function used when a process is complete.
         *
         * @return ProcessCompleteCallback object
         */
		ProcessCompleteCallback GetProcessCompleteCallback() { return mProcessCompleteCallback; }
		
        /**
         * SetCurrentWorkingDirectory() sets the current working directory the child process should use. This can only be called
         * for ProcessHandles that have not already been run.
         *
         * @throw EProcessHandleProcessStarted
         *
         * @parame cdw string containing the directory to use as the current working directory
         */
        void SetCurrentWorkingDirectory(const FString &cwd);

        /**
         * set the enironment for the child process to use. This can only be called for
         * ProcessHandles that have not already been run.
         *
         * @throw EProcessHandleProcessStarted
         *
         * @param string-string map containing the environment variables to set
         */
        void SetEnvironment(const StrStrMap *env);

        /**
         * SetInputFilename() sets the input file to use. Can only be called on a process not yet run.
         *
         * @throw EProcessHandleProcessStarted
         *
         * @param string containing the path to the input file to use
         */
        void SetInputFilename(const FString &infile);

        /**
         * SetProcessManager() sets the ProcessManager object responsible for managing this ProcessHandle.
         * can only be called on a process that hasn't been started. This is set by the
         * ProcessManager factory function, so you shouldn't be calling this.
         *
         * @throw EProcessHandleProcessStarted
         *
         * @param pm raw pointer to a ProcessManager object.
         */
		void SetProcessManager(ProcessManager *pm);
        
        /**
         * Run() kicks off the child process by forking and execing the command-line provided at construction.
         *
         * @throw EProcessHandleUnableToOpenInputFile
         * @throw EProcessHandleUnableToOpenOutputFile
         * @throw EProcessHandleUnableToFork
         * @throw EProcessHandleUnableToDuplicateInputFD
         * @throw EProcessHandleUnableToDuplicateOutputFD
         * @throw EProcessHandleUnableToDuplicateErrorFD
         * @throw EProcessHandleExecvFailed
         * @throw EProcessHandleUnableToCloseInputFile
         * @throw EProcessHandleUnableToCloseOutputFile
         *
         * @return pid_t with the process ID of the child process
         */
        pid_t Run();

        /**
         * Wait() block until the process has finished.
         *
         * @throw EProcessHandleProcessNotRunning
         *
         * @return unsigned int holding the exit value from the child process
         */
        unsigned int Wait();

        /**
         * Cancel() sends a SIGINT signal to the child process and then blocks until it is finished
         *
         * @throw EProcessHandleProcessNotRunning
         */
        void Cancel();

        /**
         * Abandon() tells the ProcessManager to stop monitoring this ProcessHandle, optionally
         * sending the child process a SIGINT signal to terminate.
         *
         * @param signal indicates whether to send a termination signal to the process before
         * abandoning it
         *
         * @throw EProcessHandleProcessNotRunning
         */
		void Abandon(bool signal = false);

        /**
         * IsRunning() returns whether the process is currently running.
         *
         * @return bool inidcating whether the process is running
         */
        bool IsRunning();

        /**
         * SetIsRunning() is used by the ProcessManager to mark this ProcessHandle as finished
         *
         * @param running is the child process still running
         */
		void SetIsRunning(bool running);
		
        /**
         * GetGUID() returns the unique ID used to track this ProcessHandle in the ProcessManager
         *
         * @return string containing the guid
         */
		FString GetGUID() { return mGUID; }
		
        /**
         * GetChildPID() returns the process id of the child
         *
         * @return pid_t holding the child process id
         */
		pid_t GetChildPID() { return mChildPid; }
		
        /**
         * SetStatusCode() is used by the ProcessManager to set the termination status code from the child process
         */
		void SetStatusCode(unsigned int code) { mStatusCode = code; }

        /**
         * GetStatusCode() returns the status code from the terminated process
         *
         * @throw EProcessHandleProcessNotFinished
         *
         * @return unisgned int holding the child status code
         */
        unsigned int GetStatusCode();
		
        /**
         * SetProcessTerminationType() is used by the ProcessManager to set how the child process terminated
         */
		void SetProcessTerminationType(ProcessTerminationType type) { mProcessTerminationType = type; }


        /**
         * GetProcessTerminationType() returns the way the child process was terminated:
         *   ProcessExited
         *   ProcessKilled
         *   ProcessStopped
         *   ProcessUnknownTermination
         * GetProcessTerminationType() will throw an exception if it is called before 
         * the child process is finished.
         *
         * @throw EProcessHandleProcessNotFinished
         *
         * @return ProcessTerminationType
         */
		ProcessTerminationType GetProcessTerminationType();
		
        /**
         * GetOutputString() returns the output from the running process if the output file
         * parameter was set. It will throw an exception if the child is not finished. We will
         * want to be careful with this one. If you suspect the output might be large don't
         * use this function. Instead, directly access the output file yourself.
         *
         * @throw EProcessHandleProcessNotFinished
         *
         * @return string containing the output of the child process.
         */
        FString GetOutputString();
        
    private:
        
        FString mCommand;
        ProcessCompleteCallback mProcessCompleteCallback;
        FString mCurrentWorkingDirectory;
        StrStrMap mEnvironment;
        FString mInputFilename;
        FString mOutputFilename;
		
        int mInputFD;
        int mOutputFD;

		FString mGUID;
		pid_t mChildPid;
		unsigned int mStatusCode;
		ProcessTerminationType mProcessTerminationType;
		FString mOutputString;



        bool mStarted;
        bool mIsRunning;
		Mutex mFinishedLock;
		ThreadCondition mFinishedCond;

        ProcessManager *mProcessManager;
        
        /**
         * shellEscape()  scrubs the string intended for exec'ing to make sure it is safe.
         *
         * @param arg the command string to scrub
         *
         * @return a cleaned command string suitable for passing to exec
         */
        virtual FString shellEscape(const FString& arg);
    };


};
#endif
