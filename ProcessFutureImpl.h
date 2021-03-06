#ifndef __ProcessFutureImpl_h
#define __ProcessFutureImpl_h

#include "Types.h"
#include "Object.h"
#include "ProcessFuture.h"
#include "ThreadCondition.h"
#include "ProcessFuture.h"
#include "ProcessManagerImpl.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <sys/types.h>


namespace Forte
{
    class ProcessManagerImpl;

    /**
     * A handle to a process managed by ProcessManager
     */
    class ProcessFutureImpl : public ProcessFuture
    {
        friend class Forte::ProcessManagerImpl;
    public:
        typedef boost::function<void (boost::shared_ptr<ProcessFuture>)> ProcessCompleteCallback;

    protected:
        /**
         * Construct a ProcessFuture object. Instantiating the object does
         * not automatically cause the command to run. ProcessManager (a friend of
         * ProcessFuture) is the only class which should instantiate a
         * ProcessFuture.
         *
         * @param mgr The process manager which will be managing this process.
         * @param command The command you want to run
         * @param processCompleteCallback The callback to call when the process has completed
         * @param currentWorkingDirectory The directory that should be used as the current working
         * directory for the forked process.
         * @param environment a map of the environment to apply to the command
         * @param inputFilename name of the file to use for input.
         * @param commandToLog is the command that should be logged instead of the command that was run (to hide sensitive information, for example)
         */
        ProcessFutureImpl(const boost::shared_ptr<Forte::ProcessManagerImpl> &mgr,
                          const FString &command,
                          const FString &currentWorkingDirectory = "/",
                          const FString &outputFilename = "/dev/null",
                          const FString &errorFilename = "/dev/null",
                          const FString &inputFilename = "/dev/null",
                          const StrStrMap *environment = NULL,
                          const FString &commandToLog = "");

    public:
        /**
         * destructor for a ProcessFuture.
         */
        virtual ~ProcessFutureImpl();


        /**
         * set the callback function that should be executed when the
         * process is complete.
         *
         * @throw EProcessFutureStarted
         *
         * @param processCompleteCallback a ProcessCompleteCallback function object
         */
        virtual void SetProcessCompleteCallback(ProcessCompleteCallback processCompleteCallback);

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
         * @throw EProcessFutureStarted
         *
         * @param cdw string containing the directory to use as the
         * current working directory
         */
        virtual void SetCurrentWorkingDirectory(const FString &cwd);

        /**
         * set the enironment for the child process to use. This can only be called for
         * Processs that have not already been run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string-string map containing the environment variables to set
         */
        virtual void SetEnvironment(const StrStrMap *env);

        /**
         * SetInputFilename() sets the input file to use. Can only be
         * called on a process not yet run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string containing the path to the input file to use
         */
        virtual void SetInputFilename(const FString &infile);

        /**
         * SetOutputFilename() sets the output file to use. Both
         * stdout and stderr are redirected here.  Can only be called
         * on a process not yet run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string containing the path to the output file to use
         */
        virtual void SetOutputFilename(const FString &outfile);

        /**
         * SetErrorFilename() sets the output file to use.
         * stderr is redirected here.  Can only be called
         * on a process not yet run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string containing the path to the error file to use
         */
        void SetErrorFilename(const FString &errorfile);

        /**
         * GetResult() block until the process has finished, or the process is abandoned.
         *
         * @throw EProcessFutureNotRunning, or any other exception if status code is non-zero
         *
         */
        virtual void GetResult();
        virtual void GetResultTimed(const Timespec &timeout);


        /**
         * Signal() sends a signal to the child process
         *
         * @param signum signal to send
         *
         * @throw EProcessFutureNotRunning
         */
        virtual void Signal(int signum);

        /**
         * IsRunning() returns whether the process is currently running.
         *
         * @return bool inidcating whether the process is running
         */
        virtual bool IsRunning();

        /**
         * GetProcessPID() returns the process id of the child process
         *
         * @return pid_t holding the child process id
         */
         virtual pid_t GetProcessPID() { return mProcessPid; }

        /**
         * GetStatusCode() returns the status code from the terminated
         * process.  If the process was terminated abnormally via
         * signal, this is the signal number.
         *
         * @throw EProcessFutureNotFinished
         *
         * @return unisgned int holding the child status code
         */
        virtual unsigned int GetStatusCode();

        /**
         * GetProcessTerminationType() returns the way the child process was terminated:
         *   ProcessExited
         *   ProcessKilled
         *   ProcessStopped
         *   ProcessUnknownTermination
         * GetProcessTerminationType() will throw an exception if it is called before
         * the child process is finished.
         *
         * @throw EProcessFutureNotFinished
         *
         * @return ProcessTerminationType
         */
        virtual ProcessTerminationType GetProcessTerminationType();

        /**
         * GetOutputString() returns the output from the running process if the output file
         * parameter was set. It will throw an exception if the child is not finished. We will
         * want to be careful with this one. If you suspect the output might be large don't
         * use this function. Instead, directly access the output file yourself.
         *
         * @throw EProcessFutureNotFinished
         *
         * @return string containing the output of the child process.
         */
        virtual FString GetOutputString();

        /**
         * GetErrorString() returns the error output from the running process if the error file
         * parameter was set. It will throw an exception if the child is not finished. We will
         * want to be careful with this one. If you suspect the output might be large don't
         * use this function. Instead, directly access the output file yourself.
         *
         * @throw EProcessFutureNotFinished
         *
         * @return string containing the output of the child process.
         */
        FString GetErrorString();

        /**
         * Cancel() sends signal 15 to the running process.
         *
         * @throw EProcessFutureNotRunning
         */
        virtual void Cancel();

        /**
         * GetMonitorPID() returns the pid for the monitor
         *
         * @return the pid of the monitor process
         */
        virtual pid_t GetMonitorPID() { return mMonitorPid; }

        virtual void SetManagementChannel(boost::shared_ptr<PDUPeer> managementChannel)
        {
            mManagementChannel = managementChannel;
        }

        virtual FString GetCommand() { return mCommand; }

    protected:


        /**
         * Get a shared pointer to the ProcessManager for this handle.
         *
         * @return shared_ptr to the process manager
         */
        boost::shared_ptr<ProcessManagerImpl> getProcessManager(void);

        /**
         * run() kicks off the child process by forking and execing
         * the command-line provided at construction.
         *
         * @throw EProcessFutureStarted
         * @throw EProcessFutureUnableToOpenInputFile
         * @throw EProcessFutureUnableToOpenOutputFile
         * @throw EProcessFutureUnableToFork
         * @throw EProcessFutureUnableToDuplicateInputFD
         * @throw EProcessFutureUnableToDuplicateOutputFD
         * @throw EProcessFutureUnableToDuplicateErrorFD
         * @throw EProcessFutureExecvFailed
         * @throw EProcessFutureUnableToCloseInputFile
         * @throw EProcessFutureUnableToCloseOutputFile
         *
         * @return pid_t with the process ID of the child process
         */
        void run();

        /**
         * abandon() tells the ProcessFutureManager to stop monitoring this
         * Process. The ProcessFuture handle will no longer be valid
         * following this call.
         *
         */
        void abandon();

        /**
         * Set the internal state of the process handle.  This will
         * wake up anyone waiting for a state change.
         *
         * @param state
         */
        void setState(int state);

        int getState() const {
            AutoUnlockMutex lock(mStateLock);
            return mState;
        }

        bool isInTerminalState(void) {
            int state = getState();
            return (state == STATE_ERROR ||
                    state == STATE_EXITED ||
                    state == STATE_KILLED ||
                    state == STATE_ABANDONED);
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
            int state = getState();
            return (state == STATE_STARTING ||
                    state == STATE_RUNNING ||
                    state == STATE_STOPPED);
        }

        /**
         * Return true if the process is definitively in a 'running'
         * state, where running is defined as the process has already
         * started, and has not yet terminated in any way.
         *
         * @return bool
         */
        bool isInRunningState(void) {
            int state = getState();
            return (state == STATE_RUNNING ||
                    state == STATE_STOPPED);
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
                throw EProcessFuture(); // \TODO more specific
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

        /**
         * Called when an unrecoverable error has occurred on the
         * connection to a peer.
         *
         * @param peer
         */
        void handleError(PDUPeer &peer);

    private:
        boost::weak_ptr<ProcessManagerImpl> mProcessManagerPtr;

        boost::shared_ptr<PDUPeer> mManagementChannel;

        FString mCommand;
        FString mCommandToLog;
        ProcessCompleteCallback mProcessCompleteCallback;
        FString mCurrentWorkingDirectory;

        StrStrMap mEnvironment;
        FString mOutputFilename;
        FString mErrorFilename;
        FString mInputFilename;

        pid_t mMonitorPid;
        pid_t mProcessPid;
        unsigned int mStatusCode;
        FString mOutputString;
        FString mErrorString;

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
        mutable Mutex mStateLock;

        Mutex mWaitLock;
        ThreadCondition mWaitCond;
    };

    typedef boost::shared_ptr<ProcessFuture> ProcessFuturePtr;
};
#endif
