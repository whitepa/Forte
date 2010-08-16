#include "LogManager.h"
#include "ProcessMonitor.h"

/**
 * procmon is a process which is started by the ProcManager class for
 * the purpose of monitoring a sub-process.  A socket connection is
 * maintained with the originating ProcManager instance, and status
 * and output are communicated back to the ProcManager via the Forte
 * PDUPeer system.
 */
int main(int argc, char *argv[])
{
    try
    {
        Forte::ProcessMonitor pm(argc, argv);
        pm.Run();
        return 0;
    }
    catch (...)
    {
        // unhandled exception
        return 1;
    }
}
