#ifndef __ProcessManager_h
#define __ProcessManager_h

#include "Thread.h"
#include "Types.h"
#include "Object.h"
#include "Clock.h"
#include <boost/pointer_cast.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <sys/types.h>

namespace Forte
{
    class ProcessFuture;

    EXCEPTION_CLASS(EProcessManager);
    EXCEPTION_SUBCLASS2(EProcessManager, EProcessManagerUnableToFork,
                        "Unable to fork a child process");
    EXCEPTION_SUBCLASS2(EProcessManager, EProcessManagerUnableToCreateProcmon,
                        "Unable to create a procmon process");
    EXCEPTION_SUBCLASS2(EProcessManager, EProcessManagerInvalidPeer,
                        "Received message from an invalid peer");
    EXCEPTION_SUBCLASS2(EProcessManager, EProcessManagerNoSharedPtr,
                        "No shared pointer exists to this process manager");
    EXCEPTION_SUBCLASS2(EProcessManager, EProcessManagerUnableToCreateSocket,
                        "Unable to create a socketpair");

    /**
     * ProcessManager provides for the creation and management of
     * child processes in an async manner
     */
    class ProcessManager : virtual public Object
    {
        friend class Forte::ProcessFuture;
    public:

        static const int MAX_RUNNING_PROCS;
        static const int PDU_BUFFER_SIZE;

        typedef std::map<int, boost::weak_ptr<ProcessFuture> > ProcessMap;

        ProcessManager()
        {
        }

        /**
         * ProcessManager destructor. If the process manager is being destroyed it will
         * notify all running Process Wait()'ers, and mark the process termination
         * type as ProcessNotTerminated.
         */
        virtual ~ProcessManager()
        {
        }


        /**
         * Get a shared pointer to this ProcessManager.  NOTE: A
         * shared_ptr to this ProcessManager must already exist.
         *
         * @return shared_ptr
         */
        boost::shared_ptr<ProcessManager> GetPtr(void) {
            try
            {
                return boost::shared_dynamic_cast<ProcessManager>(shared_from_this());
            }
            catch (boost::bad_weak_ptr &e)
            {
                throw EProcessManagerNoSharedPtr();
            }
        }


        /**
         * CreateProcess() is the factory function to create a
         * Process object representing a child process. The
         * Process returned is in running state. Once you
         * have created the Process with the ProcessManager
         * factory method, you manage the child through the
         * Process.
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
         * @param errorFilename string containing the file to use for
         * writing error output from the child process. Default value is
         * /dev/null
         * @param environment a string-string map holding environment
         * variables that will be applied to the child process.  Note
         * that these variables are ADDED to the current environment.
         * @param commandToLog the command string to log in a log file
         * if different than the command
         *
         * @return a shared pointer holding a Process object.
         */
        virtual boost::shared_ptr<ProcessFuture> CreateProcess(
            const FString &command,
            const FString &currentWorkingDirectory = "/",
            const FString &outputFilename = "/dev/null",
            const FString &errorFilename = "/dev/null",
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL,
            const FString &commandToLog = "") = 0;

        virtual boost::shared_ptr<ProcessFuture> CreateProcessDontRun(
            const FString &command,
            const FString &currentWorkingDirectory = "/",
            const FString &outputFilename = "/dev/null",
            const FString &errorFilename = "/dev/null",
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL,
            const FString &commandToLog = "") = 0;

        virtual void RunProcess(boost::shared_ptr<ProcessFuture> ph) = 0;

        virtual bool IsProcessMapEmpty(void) = 0;

        virtual int CreateProcessAndGetResult(
            const Forte::FString& command,
            Forte::FString& output,
            Forte::FString& errorOutput,
            bool throwErrorOnNonZeroReturnCode = true,
            const Timespec &timeout = Timespec::FromSeconds(-1),
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL,
            const Forte::FString& commandToLog= "") = 0;
    };
    typedef boost::shared_ptr<ProcessManager> ProcessManagerPtr;
};
#endif
