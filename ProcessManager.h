#ifndef __ProcessManager_h
#define __ProcessManager_h

#include "Types.h"
#include "Object.h"
#include "Thread.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <sys/types.h>

namespace Forte
{


    class ProcessHandle;

    EXCEPTION_CLASS(EProcessManager);
    EXCEPTION_SUBCLASS2(EProcessManager, EProcessManagerUnableToFork, "Unable fork a child process");

    /**
     * ProcessManager provides for the creation and management of
     * child processes in an async manner
     */
    class ProcessManager : public Thread
    {
    public:
        typedef std::map<FString, boost::shared_ptr<ProcessHandle> > ProcessHandleMap;
        typedef std::map<pid_t, boost::shared_ptr<ProcessHandle> > RunningProcessHandleMap;
        typedef std::pair<pid_t, boost::shared_ptr<ProcessHandle> > ProcessHandlePair;
		
        ProcessManager();

        /**
         * ProcessManager destructor. If the process manager is being destroyed it will
         * notify all running ProcessHandle Wait()'ers, and mark the process termination
         * type as ProcessNotTerminated.
         */
        virtual ~ProcessManager();

        /**
         * CreateProcess() is the factory function to create a
         * ProcessHandle object representing a child process. The
         * ProcessHandle returned is in a non-running state. Once you
         * have created the ProcessHandle with the ProcessManager
         * factory method, you manage the child through the
         * ProcessHandle.
         *
         * @param command string containing the command to run in the
         * child process
         * @param currentWorkingDirectory string containing the
         * working directory from which to run the command. default
         * value is the root directory (/).
         * @param inputFilename string containing the file the child
         * should use for input. Default value is /dev/null
         * @param outputFilename string containing the file to use for
         * writing output from the child process. Default value is
         * /dev/null
         * @param environment a string-string map holding environment
         * variables that will be applied to the child process.  Note
         * that these variables are ADDED to the current environment.
         *
         * @return a shared pointer holding a ProcessHandle object.
         */
        virtual boost::shared_ptr<ProcessHandle> CreateProcess(
            const FString &command,
            const FString &currentWorkingDirectory = "/",
            const FString &outputFilename = "/dev/null",
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL);


        /**
         * helper method called by ProcessHandle objects to notify the
         * ProcessManager a child process is running.
         */
        virtual void RunProcess(const FString &guid);

        /**
         * helper function called by ProcessHandle objects to notify
         * the ProcessManager that a child process is being abandoned
         */
        virtual void AbandonProcess(const FString &guid);
		
    private:
        /**
         * the main runloop for the ProcessManager thread monitoring
         * child processes.
         */
        virtual void * run(void);

        ProcessHandleMap processHandles;
        RunningProcessHandleMap runningProcessHandles;
        Mutex mLock;
    };

};
#endif
