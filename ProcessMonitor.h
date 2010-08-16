#ifndef _Forte_process_monitor_h_
#define _Forte_process_monitor_h_

#include "FString.h"
#include "LogManager.h"
#include "PDUPeerSet.h"
#include "Process.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(EProcess, EProcessMonitor);
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorArguments,
                        "Incorrect command line arguments");

    class ProcessMonitor
    {
    public:
        enum State
        {
            STATE_STARTUP,
            STATE_READY,
            STATE_RUNNING,
            STATE_STOPPED,
            STATE_EXITED
        };

        /** 
         * Instantiate a ProcessMonitor application.  This will
         * communicate over a socket with a Forte ProcessManager.
         * Command line usage:
         *
         * procmon <fd>
         *
         * @param argc 
         * @param argv 
         *
         * @return 
         */
        ProcessMonitor(int argc, char *argv[]);
        virtual ~ProcessMonitor();
        
        /** 
         * Run the monitoring routine.
         * 
         */
        void Run(void);

    private:
        
        /** 
         * This function gets called whenever a PDU is received on any
         * peer.
         * 
         * @param peer The peer we received the PDU from.
         * @param pdu The PDU itself.
         */
        void pduCallback(const PDUPeer &peer, const PDU &pdu);

        /** 
         * This function is called after a SIGCHLD is received, once
         * we have returned from the async signal handler.
         * 
         */
        void doWait(void);


        /** 
         * Called after a waitpid() call gives us some data.
         * 
         * @param pid 
         * @param status 
         */
        void handleWaitStatus(pid_t pid, int status);
            

        /** 
         * Async signal handler function for the management process.
         * 
         * @param sig 
         */
        static void handleSIGCHLD(int sig);


        void handlePrepare(const PDUPeer &peer, const PDU &pdu);
        void handleControlReq(const PDUPeer &peer, const PDU &pdu);
        void sendControlRes(const PDUPeer &peer, int resultCode, const char *desc = "");

        void startProcess(void);
        void signalProcess(int signal);

        /**
         * shellEscape()  scrubs the string intended for exec'ing to make sure it is safe.
         *
         * @param arg the command string to scrub
         *
         * @return a cleaned command string suitable for passing to exec
         */
        virtual Forte::FString shellEscape(const FString& arg);



        /**
         * This pointer is used by the handleSIGCHLD() function in the
         * management process only.
         */
        static ProcessMonitor *sInstancePtr;

        /**
         * A log manager, so we can log.
         */
        Forte::LogManager mLogManager;

        /**
         * flag to indicate a SIGCHLD has been received, and wait()
         * must be called.
         */
        bool mGotSIGCHLD;

        /**
         * epoll descriptor
         */
        int mPollFD;

        /**
         * Process ID of child.
         */
        pid_t mPID;

        /**
         * GUID given to us by the ProcessManager
         */
        FString mGUID;

        /**
         * Peers we may be sending/receiving PDUs to/from.
         */
        PDUPeerSet mPeerSet;

        /**
         * Current state of the monitored process.
         */
        int mState;

        FString mCmdline;
        FString mCWD;
        FString mInputFilename;
        FString mOutputFilename;
    };
};

#endif
