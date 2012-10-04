#ifndef __ProcessManagerImpl_h
#define __ProcessManagerImpl_h

#include "Thread.h"
#include "Types.h"
#include "PDUPeerSetImpl.h"
#include "Object.h"
#include "ProcessManager.h"
#include "ProcessFutureImpl.h"
#include "ProcessManagerPDU.h"
#include "Clock.h"
#include <boost/pointer_cast.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <sys/types.h>

namespace Forte
{
    class ProcessFutureImpl;

    /**
     * ProcessManager provides for the creation and management of
     * child processes in an async manner
     */
    class ProcessManagerImpl : virtual public ProcessManager,
                               virtual public Thread
    {
        friend class Forte::ProcessFutureImpl;
    protected:
 
    public:

        static const int MAX_RUNNING_PROCS;
        static const int PDU_BUFFER_SIZE;

        typedef std::map<int, boost::weak_ptr<ProcessFutureImpl> > ProcessMap;

        ProcessManagerImpl();

        /**
         * ProcessManagerImpl destructor. If the process manager is being destroyed it will
         * notify all running Process Wait()'ers, and mark the process termination
         * type as ProcessNotTerminated.
         */
        virtual ~ProcessManagerImpl();


        /**
         * Get a shared pointer to this ProcessManager.  NOTE: A
         * shared_ptr to this ProcessManager must already exist.
         * 
         * @return shared_ptr
         */
        boost::shared_ptr<ProcessManager> GetPtr(void) {
            try
            {
                return boost::dynamic_pointer_cast<ProcessManager>(shared_from_this());
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

        virtual void RunProcess(boost::shared_ptr<ProcessFuture> ph);

        virtual const FString & GetProcmonPath(void) { return mProcmonPath; }

        virtual bool IsProcessMapEmpty(void);

        virtual int CreateProcessAndGetResult(
            const Forte::FString& command, 
            Forte::FString& output, 
            Forte::FString& errorOutput,
            bool throwErrorOnNonZeroReturnCode = true,
            const Timespec &timeout = Timespec::FromSeconds(-1),
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL,
            const FString &commandToLog = "");
    protected:

        /**
         * helper function called by Process objects to notify
         * the ProcessManager that a child process is being abandoned
         *
         * @param fd file descriptor connected to the process being abandoned
         */
        virtual void abandonProcess(const boost::shared_ptr<Forte::PDUPeer> &peer);

        virtual void startMonitor(boost::shared_ptr<Forte::ProcessFutureImpl> ph);

        /**
         * the main runloop for the ProcessManager thread monitoring
         * child processes.
         */
        virtual void * run(void);

        /** 
         * addPeer() is used by new Process objects after creating
         * their monitoring process.  The ProcessManager object (which
         * owns the Process object) is responsible for polling the
         * associated file descriptor.
         * 
         * @param fd 
         * @return shared pointer to the newly created PDU Peer
         */
        virtual boost::shared_ptr<Forte::PDUPeer> addPeer(int fd);

        /**
         * This function gets called whenever a PDU is received on any
         * peer.
         * 
         * @param peer The peer we received the PDU from.
         * @param pdu The PDU itself.
         */
        void pduCallback(PDUPeer &peer);

        /** 
         * This function gets called when an error is encountered on a
         * peer connection and the peer is removed from the
         * PDUPeerSetImpl.
         * 
         * @param peer 
         */
        void errorCallback(PDUPeer &peer);

        /**
         * Lock for the process map
         */
        Mutex mProcessesLock;

        /**
         * a map of processes, indexed by process ID
         */
        ProcessMap mProcesses;

        /**
         * Set of PDU peers
         */
        PDUPeerSetImpl mPeerSet;

        FString mProcmonPath;

    };
    typedef boost::shared_ptr<ProcessManager> ProcessManagerPtr;
};
#endif
