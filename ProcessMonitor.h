#ifndef _Forte_process_monitor_h_
#define _Forte_process_monitor_h_

#include "FString.h"
#include "LogManager.h"
#include "PDUPeerSet.h"
#include "ProcessFuture.h"
#include "ServiceConfig.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(EProcessFuture, EProcessMonitor);
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorArguments,
                        "Incorrect command line arguments");
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorArgumentsTooLong,
                        "Size of arguments and environment exceeds maximum");
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorUnableToOpenInputFile,
                        "Unable to open input file");
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorUnableToOpenOutputFile,
                        "Unable to open output file");
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorUnableToOpenErrorFile,
                        "Unable to open error file");
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorUnableToCWD,
                        "Unable to change to working directory");
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorUnableToFork,
                        "Unable to fork a new process");
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorUnableToExec,
                        "Unable to execute command");
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorUnknownMessage,
                        "Received an unsupported/unknown message");
    EXCEPTION_SUBCLASS2(EProcessMonitor, EProcessMonitorInvalidState,
                        "Process Monitor in invalid state");

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
        
        bool isActiveState(void) {
            return (mState == STATE_RUNNING ||
                    mState == STATE_STOPPED);
        }

        /** 
         * This function gets called whenever a PDU is received on any
         * peer.
         * 
         * @param peer The peer we received the PDU from.
         * @param pdu The PDU itself.
         */
        void pduCallback(PDUPeer &peer);

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


        void handleParam(const PDUPeer &peer, const PDU &pdu);
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
         * A log manager, so we can log.
         */
        Forte::LogManager mLogManager;

        /**
         * ServiceConfig to load the configuration file.
         */
        Forte::ServiceConfig mServiceConfig;

        /**
         * flag to indicate a SIGCHLD has been received, and wait()
         * must be called.
         */
        static bool sGotSIGCHLD;

        /**
         * epoll descriptor
         */
        int mPollFD;

        /**
         * Process ID of child.
         */
        pid_t mPID;

        /**
         * Peers we may be sending/receiving PDUs to/from.
         */
        PDUPeerSet mPeerSet;

        /**
         * Current state of the monitored process.
         */
        int mState;

        FString mCmdline;
        FString mCmdlineToLog;
        FString mCWD;
        FString mInputFilename;
        FString mOutputFilename;
        FString mErrorFilename;
    };
};

#endif
