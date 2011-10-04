#ifndef _Forte_Process_Manager_PDU_h_
#define _Forte_Process_Manager_PDU_h_

namespace Forte
{

    enum PDUOpcodes
    {
        ProcessOpParam,
        ProcessOpStatus,
        ProcessOpOutput,
        ProcessOpControlReq,
        ProcessOpControlRes,
        ProcessOpInfoReq,
        ProcessOpInfoRes
    };

    enum ProcessStatusType
    {
        ProcessStatusStarted,
        ProcessStatusError,
        ProcessStatusExited,
        ProcessStatusKilled,
        ProcessStatusStopped,
        ProcessStatusContinued,
        ProcessStatusUnknownTermination,
        ProcessStatusNotTerminated
    };

    enum ProcessParamCode
    {
        ProcessCmdline,
        ProcessCwd,
        ProcessInfile,
        ProcessOutfile,
        ProcessErrfile
    };

    struct ProcessParamPDU
    {
        int param;
        char str[2048];
    } __attribute__((__packed__));
    
    struct ProcessStatusPDU
    {
        int type;   // started, error, exited, killed, stopped
        int statusCode; // exit code, signal # delivered, or error type
        struct timeval timestamp;
        int msgLen;
        char msg[1024];
    } __attribute__((__packed__));
    struct ProcessOutputPDU
    {
        int len;
        char data[1024];
    } __attribute__((__packed__));

    enum ProcessControlCodes
    {
        ProcessControlStart,
        ProcessControlSignal
    };
    struct ProcessControlReqPDU
    {
        int control;   // start, signal
        int signum;
    } __attribute__((__packed__));

    enum ProcessControlResultCodes
    {
        ProcessSuccess,
        ProcessUnableToOpenInputFile,
        ProcessUnableToOpenOutputFile,
        ProcessUnableToOpenErrorFile,
        ProcessUnableToCWD,
        ProcessUnableToFork,
        ProcessUnableToExec,
        ProcessNotRunning,
        ProcessRunning,
        ProcessProcmonFailure,
        ProcessUnknownError
    };
    struct ProcessControlResPDU
    {
        int result; // 0 = success
        pid_t monitorPID;
        pid_t processPID;
        char error[1024]; // description of error
    } __attribute__((__packed__));
    struct ProcessInfoReqPDU
    {
        int nothing;
    };
    struct ProcessInfoResPDU
    {
        // started by process name (short name)
        char startedBy[64];
        // started by PID
        int startedByPID;
        // start timestamp
        struct timeval startTime;
        // elapsed time
        struct timeval elapsed;
        // command line
        char cmdline[2048];
        // cwd
        char cwd[1024];
        // monitor PID
        int monitorPID;
        // process PID
        int processPID;
    } __attribute__((__packed__));
};
#endif
