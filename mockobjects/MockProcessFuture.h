#ifndef __MockProcessFuture_h
#define __MockProcessFuture_h

#include "Types.h"
#include "Object.h"
#include "ProcessFuture.h"
#include "MockProcessManager.h"
#include "ThreadCondition.h"
#include "ProcessManager.h"
#include "FileSystem.h"
#include "Clock.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <sys/types.h>


namespace Forte
{
    EXCEPTION_SUBCLASS2(EUnimplemented, EMockProcessFutureUnimplemented,
                        "Method not implemented by MockProcessManager");
    EXCEPTION_SUBCLASS2(EUnimplemented, EMockProcessFutureCancelUnimplemented,
                        "Method not implemented by MockProcessManager");

    /**
     * A handle to a process managed by ProcessManager
     */
    class MockProcessFuture : public ProcessFuture
    {

    public:
        MockProcessFuture(const FString &command,
                          const FString &outputFilename,
                          const FString &inputFilename,
                          const ExpectedCommandResponsePtr &response,
                          const boost::shared_ptr<Forte::FileSystem> &fs) :
            mCommand(command),
            mOutputFilename(outputFilename),
            mInputFilename(inputFilename),
            mResponse(response),
            mFS(fs) {
        };

        virtual ~MockProcessFuture() {};

        /**
         * set the callback function that should be executed when the
         * process is complete.
         *
         * @throw EProcessFutureStarted
         *
         * @param processCompleteCallback a ProcessCompleteCallback function object
         */
        virtual void SetProcessCompleteCallback(ProcessCompleteCallback processCompleteCallback) {
            throw EMockProcessFutureUnimplemented();
        }

        /**
         * GetProcessCompleteCallback() returns the callback function
         * used when a process is complete.
         *
         * @return ProcessCompleteCallback object
         */
        virtual ProcessCompleteCallback GetProcessCompleteCallback() {
            throw EMockProcessFutureUnimplemented();
        }

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
        virtual void SetCurrentWorkingDirectory(const FString &cwd) {};

        /**
         * set the enironment for the child process to use. This can only be called for
         * Processs that have not already been run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string-string map containing the environment variables to set
         */
        virtual void SetEnvironment(const StrStrMap *env) {};

        /**
         * SetInputFilename() sets the input file to use. Can only be
         * called on a process not yet run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string containing the path to the input file to use
         */
        virtual void SetInputFilename(const FString &infile) {};

        /**
         * SetOutputFilename() sets the output file to use. Both
         * stdout and stderr are redirected here.  Can only be called
         * on a process not yet run.
         *
         * @throw EProcessFutureStarted
         *
         * @param string containing the path to the output file to use
         */
        virtual void SetOutputFilename(const FString &outfile) {};

        /**
         * GetResult() block until the process has finished, or the process is abandoned.
         *
         * @throw EProcessFutureNotRunning, or any other exception if status code is non-zero
         *
         */
        virtual void GetResult();
        virtual void GetResultTimed(const Forte::Timespec &timeout);


        /**
         * Signal() sends a signal to the child process
         *
         * @param signum signal to send
         *
         * @throw EProcessFutureNotRunning
         */
        virtual void Signal(int signum) { throw EMockProcessFutureUnimplemented(); }

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
        virtual pid_t GetProcessPID() { throw EMockProcessFutureUnimplemented(); }

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
        virtual ProcessTerminationType GetProcessTerminationType() {
            throw EMockProcessFutureUnimplemented();
        }

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

        virtual FString GetErrorString();
        virtual void SetErrorFilename(const FString &errorfile);

        /**
         * Cancel() sends signal 15 to the running process.
         *
         * @throw EProcessFutureNotRunning
         */
        virtual void Cancel() { throw EMockProcessFutureCancelUnimplemented(); }

        /**
         * GetMonitorPID() returns the pid for the monitor
         *
         * @return the pid of the monitor process
         */
        virtual pid_t GetMonitorPID() { throw EMockProcessFutureUnimplemented(); }

        virtual void SetManagementChannel(boost::shared_ptr<PDUPeer> managementChannel) {
            throw EMockProcessFutureUnimplemented();
        }

        virtual FString GetCommand() {
            AutoUnlockMutex lock(mLock);
            return mCommand;
        }

        // routines specific to the MockProcessFuture:
        inline void Start() {
            mStartTime = Forte::MonotonicClock().GetTime();
        }
        inline bool HasStarted() {
            return !mStartTime.IsZero();
        }
    protected:
        Mutex mLock;
        Forte::FString mCommand;
        FString mOutputFilename;
        FString mInputFilename;
        unsigned int mStatusCode;
        ExpectedCommandResponsePtr mResponse;
        boost::shared_ptr<Forte::FileSystem> mFS;

        Forte::Timespec mStartTime;
    };

    typedef boost::shared_ptr<ProcessFuture> ProcessFuturePtr;
};
#endif
