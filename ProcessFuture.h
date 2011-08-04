#ifndef __ProcessFuture_h
#define __ProcessFuture_h

#include "Types.h"
#include "Object.h"
#include "Future.h"
#include "ThreadCondition.h"
#include "ProcessManager.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <sys/types.h>


namespace Forte
{

    EXCEPTION_CLASS(EProcessFuture);
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToOpenInputFile,
                        "Unable to open the input file");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToCloseInputFile,
                        "Unable to close the input file");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToOpenOutputFile,
                        "Unable to open the output file");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToCloseOutputFile,
                        "Unable to close the output file");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToOpenErrorFile,
                        "Unable to open the error file");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToCloseErrorFile,
                        "Unable to close the error file");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToCWD,
                        "Unable to change to working directory");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToFork,
                        "Unable to Fork Child ProcessFuture");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToExec,
                        "Unable to Exec Child Process");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToCreateSocket,
                        "Unable to create a socketpair");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureNotRunning,
                        "Wait called on a non-running process");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureRunning,
                        "method called on a running process");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureStarted,
                        "method called on a process that has already started");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureNotStarted,
                        "method called on a process that has not been started");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureNotFinished,
                        "method called on a process that is not finished yet");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureNoExit,
                        "ProcessFuture did not exit");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureAbandoned,
                        "Process has been abandoned");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToDuplicateInputFD,
                        "Unable to duplicate Input File Descriptor");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToDuplicateOutputFD,
                        "Unable to duplicate Output File Descriptor");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnableToDuplicateErrorFD,
                        "Unable to duplicate Error File Descriptor");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureSignalFailed,
                        "Unable to signal process");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureHandleInvalid,
                        "Process handle is invalid");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureManagementProcFailed,
                        "An error occurred in the management process");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureUnknownState,
                        "Process is in unknown state");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureTerminatedWithNonZeroStatus,
                        "The process terminated with a non-zero status code");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureKilled,
                        "The process was killed");
    EXCEPTION_SUBCLASS2(EProcessFuture, EProcessFutureTerminatedDueToUnknownReason,
                        "The process terminated due to unknown reason");

    /**
     * A handle to a process managed by ProcessManager
     */
    class ProcessFuture : public Future<void>
    {
        friend class Forte::ProcessManager;
    public:
        typedef boost::function<void (boost::shared_ptr<ProcessFuture>)> ProcessCompleteCallback;
        
        enum ProcessTerminationType
        {
            ProcessExited,
            ProcessKilled,
            ProcessStopped,            // \TODO not a terminal state
            ProcessUnknownTermination,
            ProcessNotTerminated       // \TODO not a terminal state
        };
        ProcessFuture() {};
        virtual ~ProcessFuture() {};

        /**
         * set the callback function that should be executed when the
         * process is complete.
         *
         * @throw EProcessFutureStarted
         *
         * @param processCompleteCallback a ProcessCompleteCallback function object
         */
        virtual void SetProcessCompleteCallback(ProcessCompleteCallback processCompleteCallback) = 0;

        /**
         * GetProcessCompleteCallback() returns the callback function
         * used when a process is complete.
         *
         * @return ProcessCompleteCallback object
         */
        virtual ProcessCompleteCallback GetProcessCompleteCallback() = 0;
  
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
        virtual void SetCurrentWorkingDirectory(const FString &cwd) = 0;

        /**
         * set the enironment for the child process to use. This can only be called for
         * Processs that have not already been run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string-string map containing the environment variables to set
         */
        virtual void SetEnvironment(const StrStrMap *env) = 0;

        /**
         * SetInputFilename() sets the input file to use. Can only be
         * called on a process not yet run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string containing the path to the input file to use
         */
        virtual void SetInputFilename(const FString &infile) = 0;

        /**
         * SetOutputFilename() sets the output file to use.
         * stdout is redirected here.  Can only be called
         * on a process not yet run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string containing the path to the output file to use
         */
        virtual void SetOutputFilename(const FString &outfile) = 0;

        /**
         * SetErrorFilename() sets the output file to use. 
         * stderr is redirected here.  Can only be called
         * on a process not yet run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string containing the path to the error file to use
         */
        virtual void SetErrorFilename(const FString &errorfile) = 0;

        /**
         * GetResult() block until the process has finished, or the process is abandoned.
         *
         * @throw EProcessFutureNotRunning, or any other exception if status code is non-zero
         *
         */
        virtual void GetResult() = 0;

        virtual void GetResultTimed(const Timespec &timeout) = 0;


        /**
         * Signal() sends a signal to the child process
         *
         * @param signum signal to send
         *
         * @throw EProcessFutureNotRunning
         */
        virtual void Signal(int signum) = 0;

        /**
         * IsRunning() returns whether the process is currently running.
         *
         * @return bool inidcating whether the process is running
         */
        virtual bool IsRunning() = 0;

        /**
         * GetProcessPID() returns the process id of the child process
         *
         * @return pid_t holding the child process id
         */
        virtual pid_t GetProcessPID() = 0;
  
        /**
         * GetStatusCode() returns the status code from the terminated
         * process.  If the process was terminated abnormally via
         * signal, this is the signal number.
         *
         * @throw EProcessFutureNotFinished
         *
         * @return unisgned int holding the child status code
         */
        virtual unsigned int GetStatusCode() = 0;
  
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
        virtual ProcessTerminationType GetProcessTerminationType() = 0;

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
        virtual FString GetOutputString() = 0;

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
        virtual FString GetErrorString() = 0;

        /**
         * Cancel() sends signal 15 to the running process.
         * 
         * @throw EProcessFutureNotRunning
         */
        virtual void Cancel() = 0;

        /**
         * GetMonitorPID() returns the pid for the monitor
         *
         * @return the pid of the monitor process
         */
        virtual pid_t GetMonitorPID() = 0;

        virtual void SetManagementChannel(boost::shared_ptr<PDUPeer> managementChannel) = 0;

        virtual FString GetCommand() = 0;
                
    protected:
    };

    typedef boost::shared_ptr<ProcessFuture> ProcessFuturePtr;
};
#endif
