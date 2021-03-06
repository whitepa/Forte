#ifndef _Pidfile_h
#define _Pidfile_h

#include "Exception.h"
#include "FString.h"
#include "FileSystem.h"

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
        PidFile(FileSystemPtr fs,
                const FString &pidFilePath,
                const FString &executableName);
        virtual ~PidFile();

        /*
         * Checks that the PID found in the PID file provided is
         * associated with a running process.  Also, verifies that the
         * name of the running process matches the name of the
         * executable.
         *
         * @return true when a process is running with the PID found in mPath
         */
        bool IsProcessRunning();

        /*
         * Creates the PID file with the PID provided
         * @param pid  the PID of the process to put in the file
         *             (use 0 to just use the current process' pid)
         * @throws EAlreadyRunning if the process is already running
         * @htrows EPidFileOpenFailed when unable to write to the PID file
         */
        void Create(unsigned int pid=0);

        /*
         * Deletes the PID file
         */
        void Delete(void);

    protected:
        FString mPidFilePath;
        FileSystemPtr mFileSystem;
        FString mExecutableName;

        /*
         * Flag to indicate that this pid file was created and should be
         * removed when this object is destructed
         */
        bool mShouldDeleteWhenObjectDestroyed;
    };
};
#endif
