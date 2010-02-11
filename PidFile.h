#ifndef _Pidfile_h
#define _Pidfile_h

EXCEPTION_SUBCLASS2(CException, EAlreadyRunning, "Process is already running");
EXCEPTION_SUBCLASS2(CException, EPidfileOpenFailed, "Failed to create pidfile");

class CPidFile
{
public:
    /// Create a PID file in the filesystem at the given path.
    /// Exceptions will be thrown if:
    ///  PID file already exists and is referenced by a running process
    ///  Desired directory for pidfile does not exist or cannot be written
    ///
    ///
    CPidFile(const char *path);

    virtual ~CPidFile();

protected:
    FString mPath;
};

#endif
