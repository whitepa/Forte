#ifndef __Process_h
#define __Process_h

#include "Types.h"
#include "Object.h"
#include "ThreadCondition.h"
#include "ProcessManager.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <sys/types.h>


namespace Forte
{

    EXCEPTION_CLASS(EProcess);
    EXCEPTION_SUBCLASS2(EProcess, EProcessUnableToOpenInputFile,
                        "Unable to open the input file");
    EXCEPTION_SUBCLASS2(EProcess, EProcessUnableToCloseInputFile,
                        "Unable to close the input file");
    EXCEPTION_SUBCLASS2(EProcess, EProcessUnableToOpenOutputFile,
                        "Unable to open the output file");
    EXCEPTION_SUBCLASS2(EProcess, EProcessUnableToCloseOutputFile,
                        "Unable to close the output file");
    EXCEPTION_SUBCLASS2(EProcess, EProcessUnableToFork,
                        "Unable to Fork Child Process");
    EXCEPTION_SUBCLASS2(EProcess, EProcessUnableToCreateSocket,
                        "Unable to create a socketpair");
    EXCEPTION_SUBCLASS2(EProcess, EProcessExecvFailed,
                        "Execv Call Failed");
    EXCEPTION_SUBCLASS2(EProcess, EProcessNotRunning,
                        "Wait called on a non-running process");
    EXCEPTION_SUBCLASS2(EProcess, EProcessRunning,
                        "method called on a running process");
    EXCEPTION_SUBCLASS2(EProcess, EProcessStarted,
                        "method called on a process that has already started");
    EXCEPTION_SUBCLASS2(EProcess, EProcessNotStarted,
                        "method called on a process that has not been started");
    EXCEPTION_SUBCLASS2(EProcess, EProcessNotFinished,
                        "method called on a process that is not finished yet");
    EXCEPTION_SUBCLASS2(EProcess, EProcessNoExit,
                        "Process did not exit");
    EXCEPTION_SUBCLASS2(EProcess, EProcessAbandoned,
                        "Process has been abandoned");
    EXCEPTION_SUBCLASS2(EProcess, EProcessUnableToDuplicateInputFD,
                        "Unable to duplicate Input File Descriptor");
    EXCEPTION_SUBCLASS2(EProcess, EProcessUnableToDuplicateOutputFD,
                        "Unable to duplicate Output File Descriptor");
    EXCEPTION_SUBCLASS2(EProcess, EProcessUnableToDuplicateErrorFD,
                        "Unable to duplicate Error File Descriptor");
    EXCEPTION_SUBCLASS2(EProcess, EProcessSignalFailed,
                        "Unable to signal process");
    EXCEPTION_SUBCLASS2(EProcess, EProcessHandleInvalid,
                        "Process handle is invalid");
    EXCEPTION_SUBCLASS2(EProcess, EProcessManagementProcFailed,
                        "An error occurred in the management process");
    EXCEPTION_SUBCLASS2(EProcess, EProcessUnknownState,
                        "Process is in unknown state");
    

    /**
     * A handle to a process managed by ProcessManager
     */
    class Process : public Object 
    {
        friend class Forte::ProcessManager;
    public:
        typedef boost::function<void (boost::shared_ptr<Process>)> ProcessCompleteCallback;
        
        enum ProcessTerminationType
        {
            ProcessExited,
            ProcessKilled,
            ProcessStopped,            // \TODO not a terminal state
            ProcessUnknownTermination,
            ProcessNotTerminated       // \TODO not a terminal state
        };

    protected:
        /**
         * Construct a Process object. Instantiating the object does
         * not automatically cause the command to run. You must call
         * Run to kick off the execution.  ProcessManager (a friend of
         * Process) is the only class which should instantiate a
         * Process.
         *
         * @param mgr The process manager which will be managing this process.
         * @param procmon Path to the procmon executable.
         * @param command The command you want to run
         * @param processCompleteCallback The callback to call when the process has completed
         * @param currentWorkingDirectory The directory that should be used as the current working
         * directory for the forked process.
         * @param environment a map of the environment to apply to the command
         * @param inputFilename name of the file to use for input.
         */
        Process(const boost::shared_ptr<Forte::ProcessManager> &mgr,
                const FString &procmon,
                const FString &guid,
                const FString &command,
                const FString &currentWorkingDirectory = "/",
                const FString &outputFilename = "/dev/null", 
                const FString &inputFilename = "/dev/null",
                const StrStrMap *environment = NULL);

    public:
        /**
         * destructor for a Process.
         */
        virtual ~Process();

        /** 
         * Get a shared pointer to the ProcessManager for this handle.
         * 
         * @return shared_ptr to the process manager
         */
        boost::shared_ptr<ProcessManager> GetProcessManager(void);

        
        /**
         * set the callback function that should be executed when the
         * process is complete.
         *
         * @throw EProcessStarted
         *
         * @param processCompleteCallback a ProcessCompleteCallback function object
         */
        void SetProcessCompleteCallback(ProcessCompleteCallback processCompleteCallback);

        /**
         * GetProcessCompleteCallback() returns the callback function
         * used when a process is complete.
         *
         * @return ProcessCompleteCallback object
         */
        ProcessCompleteCallback GetProcessCompleteCallback() { return mProcessCompleteCallback; }
		
        /**
         * SetCurrentWorkingDirectory() sets the current working
         * directory the child process should use. This can only be
         * called for Processs that have not already been run.
         *
         * @throw EProcessStarted
         *
         * @param cdw string containing the directory to use as the
         * current working directory
         */
        void SetCurrentWorkingDirectory(const FString &cwd);

        /**
         * set the enironment for the child process to use. This can only be called for
         * Processs that have not already been run.
         *
         * @throw EProcessStarted
         *
         * @param string-string map containing the environment variables to set
         */
        void SetEnvironment(const StrStrMap *env);

        /**
         * SetInputFilename() sets the input file to use. Can only be
         * called on a process not yet run.
         *
         * @throw EProcessStarted
         *
         * @param string containing the path to the input file to use
         */
        void SetInputFilename(const FString &infile);

        /**
         * SetOutputFilename() sets the output file to use. Both
         * stdout and stderr are redirected here.  Can only be called
         * on a process not yet run.
         *
         * @throw EProcessStarted
         *
         * @param string containing the path to the output file to use
         */
        void SetOutputFilename(const FString &outfile);
        
        /**
         * Run() kicks off the child process by forking and execing
         * the command-line provided at construction.
         *
         * @throw EProcessStarted
         * @throw EProcessUnableToOpenInputFile
         * @throw EProcessUnableToOpenOutputFile
         * @throw EProcessUnableToFork
         * @throw EProcessUnableToDuplicateInputFD
         * @throw EProcessUnableToDuplicateOutputFD
         * @throw EProcessUnableToDuplicateErrorFD
         * @throw EProcessExecvFailed
         * @throw EProcessUnableToCloseInputFile
         * @throw EProcessUnableToCloseOutputFile
         *
         * @return pid_t with the process ID of the child process
         */
        pid_t Run();


        /**
         * RunParent() is called after the fork from the ProcessManager
         */
//        void RunParent(pid_t cpid);


        /**
         * Wait() block until the process has finished, or the process is abandoned.
         *
         * @throw EProcessNotRunning
         *
         * @return unsigned int holding the exit value from the child process
         */
        unsigned int Wait();


        /**
         * Signal() sends a signal to the child process
         *
         * @param signum signal to send
         *
         * @throw EProcessNotRunning
         */
        void Signal(int signum);

        /**
         * Abandon() tells the ProcessManager to stop monitoring this
         * Process. The Process handle will no longer be valid
         * following this call.
         *
         * @throw EProcessNotRunning
         */
        void Abandon();

        /**
         * IsRunning() returns whether the process is currently running.
         *
         * @return bool inidcating whether the process is running
         */
        bool IsRunning();

        /**
         * GetGUID() returns the unique ID used to track this Process in the ProcessManager
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
         * GetStatusCode() returns the status code from the terminated
         * process.  If the process was terminated abnormally via
         * signal, this is the signal number.
         *
         * @throw EProcessNotFinished
         *
         * @return unisgned int holding the child status code
         */
        unsigned int GetStatusCode();
		
        /**
         * GetProcessTerminationType() returns the way the child process was terminated:
         *   ProcessExited
         *   ProcessKilled
         *   ProcessStopped
         *   ProcessUnknownTermination
         * GetProcessTerminationType() will throw an exception if it is called before 
         * the child process is finished.
         *
         * @throw EProcessNotFinished
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
         * @throw EProcessNotFinished
         *
         * @return string containing the output of the child process.
         */
        FString GetOutputString();
                
    protected:
        /** 
         * startMonitor is called by the ProcessManager after the
         * Process object is created.  A child process will be forked,
         * and an open socket will be established and linked via
         * PDUPeer objects.
         * 
         */
        void startMonitor(void);

        /**
         * SetIsRunning() is used by the ProcessManager to mark this Process as finished
         *
         * @param running is the child process still running
         */
        void setIsRunning(bool running);
		
        /**
         * NotifyWaiters() broadcasts for anyone waiting on the process to finish
         */
        void notifyWaiters();

        /**
         * SetStatusCode() is used by the ProcessManager to set the
         * termination status code from the child process
         */
        void setStatusCode(unsigned int code) { mStatusCode = code; }

        /**
         * SetProcessTerminationType() is used by the ProcessManager
         * to set how the child process terminated
         */
        void setProcessTerminationType(ProcessTerminationType type) { mProcessTerminationType = type; }

        /** 
         * Set the internal state of the process handle.  This will
         * wake up anyone waiting for a state change.
         * 
         * @param state 
         */
        void setState(int state);

        bool isInTerminalState(void) { 
            return (mState == STATE_ERROR ||
                    mState == STATE_EXITED ||
                    mState == STATE_KILLED ||
                    mState == STATE_ABANDONED);
        }

        /** 
         * Return true if the process is in an 'active' state, where
         * active is defined as the process possibly existing
         * (possibly because it may not have been created yet if in
         * STATE_STARTING, but it will be created shortly).
         * 
         * @return bool
         */
        bool isInActiveState(void) { 
            return (mState == STATE_STARTING ||
                    mState == STATE_RUNNING ||
                    mState == STATE_STOPPED);
        }

        /** 
         * Return true if the process is definitively in a 'running'
         * state, where running is defined as the process has already
         * started, and has not yet terminated in any way.
         * 
         * @return bool
         */
        bool isInRunningState(void) { 
            return (mState == STATE_RUNNING ||
                    mState == STATE_STOPPED);
        }

        /** 
         * Return the file descriptor of the connection to the monitor
         * process.
         * 
         * 
         * @return file descriptor
         */
        int getManagementFD(void) {
            if (!mManagementChannel)
                throw EProcess(); // \TODO more specific
            return mManagementChannel->GetFD();
        }

        /** 
         * Handle an incoming PDU
         * 
         * @param peer 
         */
        void handlePDU(PDUPeer &peer);

        void handleControlRes(PDUPeer &peer, const PDU &pdu);
        void handleStatus(PDUPeer &peer, const PDU &pdu);


    private:
        boost::weak_ptr<ProcessManager> mProcessManagerPtr;

        FString mProcmonPath;

        boost::shared_ptr<PDUPeer> mManagementChannel;

        FString mCommand;
        ProcessCompleteCallback mProcessCompleteCallback;
        FString mCurrentWorkingDirectory;

        StrStrMap mEnvironment;
        FString mOutputFilename;
        FString mInputFilename;

        int mInputFD;
        int mOutputFD;

        FString mGUID;
        pid_t mChildPid;
        unsigned int mStatusCode;
        ProcessTerminationType mProcessTerminationType;
        FString mOutputString;

        enum {
            STATE_READY,    // Process obj created
            STATE_STARTING, // ControlReq(start) sent, waiting for response
            STATE_RUNNING,  // ControlRes(success) received after ControlReq(start)
            STATE_ERROR,    // ControlRes(error) received after ControlReq(start)
            STATE_EXITED,
            STATE_KILLED,
            STATE_STOPPED,
            STATE_ABANDONED,
            STATE_UNKNOWN
        };
        int mState;

        Mutex mWaitLock;
        ThreadCondition mWaitCond;
    };
};
#endif
