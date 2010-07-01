#ifndef _Pidfile_h
#define _Pidfile_h

#include "Exception.h"
#include "FString.h"

namespace Forte
{
    EXCEPTION_SUBCLASS2(Exception, EAlreadyRunning, "Process is already running");
    EXCEPTION_SUBCLASS2(Exception, EPidfileOpenFailed, "Failed to create pidfile");

    class PidFile : public Object
    {
    public:
        /// Create a PID file in the filesystem at the given path.
        /// Exceptions will be thrown if:
        ///  PID file already exists and is referenced by a running process
        ///  Desired directory for pidfile does not exist or cannot be written
        ///
        ///
        PidFile(const char *path);

        virtual ~PidFile();

    protected:
        FString mPath;
    };
};
#endif
