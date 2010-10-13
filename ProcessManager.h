#ifndef __ProcessManager_h
#define __ProcessManager_h

#include "Thread.h"
#include "Types.h"
#include "PDUPeerSet.h"
#include "Object.h"
#include "ProcessManagerPDU.h"
#include <boost/pointer_cast.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <sys/types.h>

namespace Forte
{
    class Process;

    EXCEPTION_CLASS(EProcessManager);
    EXCEPTION_SUBCLASS2(EProcessManager, EProcessManagerUnableToFork,
                        "Unable to fork a child process");
    EXCEPTION_SUBCLASS2(EProcessManager, EProcessManagerInvalidPeer,
                        "Received message from an invalid peer");
    EXCEPTION_SUBCLASS2(EProcessManager, EProcessManagerNoSharedPtr,
                        "No shared pointer exists to this process manager");

    /**
     * ProcessManager provides for the creation and management of
     * child processes in an async manner
     */
    class ProcessManager : public Thread
    {
        friend class Forte::Process;
    protected:
 
    public:

        static const int MAX_RUNNING_PROCS;
        static const int PDU_BUFFER_SIZE;

        typedef std::map<int, boost::shared_ptr<Process> > ProcessMap;

        ProcessManager();

        /**
         * ProcessManager destructor. If the process manager is being destroyed it will
         * notify all running Process Wait()'ers, and mark the process termination
         * type as ProcessNotTerminated.
         */
        virtual ~ProcessManager();


        /**
         * Get a shared pointer to this ProcessManager.  NOTE: A
         * shared_ptr to this ProcessManager must already exist.
         * 
         * @return shared_ptr
         */
        boost::shared_ptr<ProcessManager> GetPtr(void) {
            try
            {
                return boost::static_pointer_cast<ProcessManager>(shared_from_this());
            }
            catch (boost::bad_weak_ptr &e)
            {
                throw EProcessManagerNoSharedPtr();
            }
        }


        /**
         * CreateProcess() is the factory function to create a
         * Process object representing a child process. The
         * Process returned is in a non-running state. Once you
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
        virtual boost::shared_ptr<Process> CreateProcess(
            const FString &command,
            const FString &currentWorkingDirectory = "/",
            const FString &outputFilename = "/dev/null",
            const FString &inputFilename = "/dev/null",
            const StrStrMap *environment = NULL);

        virtual const FString & GetProcmonPath(void) { return mProcmonPath; }

    private:

        /**
         * helper function called by Process objects to notify
         * the ProcessManager that a child process is being abandoned
         *
         * @param fd file descriptor connected to the process being abandoned
         */
        virtual void abandonProcess(const int fd);

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
         * a map of processes, indexed by process ID
         */
        ProcessMap mProcesses;

        /**
         * Set of PDU peers
         */
        PDUPeerSet mPeerSet;

        /**
         * Lock for this entire proc manager
         */
        Mutex mLock;

        FString mProcmonPath;
    };
};
#endif
