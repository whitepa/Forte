#include "Exception.h"
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
        boost::shared_ptr<Forte::ProcessMonitor> pm
            (new Forte::ProcessMonitor(argc, argv));
        pm->Run();
        fprintf(stderr, "PROCMON: exiting\n");
        return 0;
    }
    catch (Forte::Exception &e)
    {
        // unhandled exception
        fprintf(stderr, "PROCMON: %s\n", e.what());
        return 1;
    }
    catch (...)
    {
        // unhandled exception
        fprintf(stderr, "PROCMON: unknown exception\n");
        return 1;
    }
}
