#ifndef __ProcessManager_h
#define __ProcessManager_h

#include "Thread.h"
#include "Types.h"
#include "PDUPeerSet.h"
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
         *
         * @return a shared pointer holding a Process object.
         */
        virtual boost::shared_ptr<ProcessFuture> CreateProcess(
            const FString &command,
            const FString &currentWorkingDirectory = "/",
            const FString &outputFilename = "/dev/null",
            const FString &errorFilename = "/dev/null",
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL) = 0;

        virtual boost::shared_ptr<ProcessFuture> CreateProcessDontRun(
            const FString &command,
            const FString &currentWorkingDirectory = "/",
            const FString &outputFilename = "/dev/null",
            const FString &errorFilename = "/dev/null",
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL) = 0;

        virtual void RunProcess(boost::shared_ptr<ProcessFuture> ph) = 0;

        virtual bool IsProcessMapEmpty(void) = 0;

        virtual int CreateProcessAndGetResult(const Forte::FString& command, 
                                              Forte::FString& output, 
                                              const Timespec &timeout = Timespec::FromSeconds(-1),
                                              const FString &inputFilename = "/dev/null",
                                              const StrStrMap *environment = NULL) = 0;
    };
    typedef boost::shared_ptr<ProcessManager> ProcessManagerPtr;
};
#endif
