#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "LogManager.h"
#include "PidFile.h"
#include "FTrace.h"
#include "SystemCallUtil.h"

using namespace Forte;

PidFile::PidFile(FileSystemPtr fs,
                 const FString &pidFilePath,
                 const FString &executablePath) :
    mPidFilePath(pidFilePath),
    mFileSystem(fs),
    mExecutableName(executablePath),
    mShouldDeleteWhenObjectDestroyed(false)
{

}

void PidFile::Create(unsigned int pid)
{
    FTRACE2("%u", pid);

    if (IsProcessRunning())
    {
        throw EAlreadyRunning();
    }
    else
    {
        mFileSystem->Unlink(mPidFilePath);
    }

    if (pid == 0)
    {
        pid = getpid();
    }

    // create the pidfile (using O_EXCL to avoid race condition --
    // must be local filesystem)
    AutoFD fd;
    mFileSystem->FilePutContentsWithPerms(
        mPidFilePath,
        FString(FStringFC(), "%u", pid),
        O_CREAT | O_EXCL | O_RDWR, // 0755 perms
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH
        );

    // deal with race
    if (mFileSystem->FileGetContents(mPidFilePath).AsUInt32() != pid)
        throw EPidfileOpenFailed();

    mShouldDeleteWhenObjectDestroyed = true;

    hlog(HLOG_DEBUG, "Pid file (%s) created with pid %u.",
         mPidFilePath.c_str(), pid);
}

void PidFile::Delete(void)
{
    FTRACE;
    // delete the pidfile
    hlog(HLOG_DEBUG, "deleting pidfile");
    mFileSystem->Unlink(mPidFilePath);
}

PidFile::~PidFile()
{
    FTRACE;
    if (mShouldDeleteWhenObjectDestroyed)
    {
        Delete();
    }
}

bool PidFile::IsProcessRunning()
{
    FTRACE2("%s", mPidFilePath.c_str());

    if (mFileSystem->FileExists(mPidFilePath))
    {
        hlog(HLOG_DEBUG, "pid file %s exists. Checking for process",
             mPidFilePath.c_str());

        unsigned int pid =
            mFileSystem->FileGetContents(mPidFilePath).AsUnsignedInteger();

        FString procCmdLine(FStringFC(), "/proc/%u/cmdline", pid);

        if (mFileSystem->FileExists(procCmdLine))
        {
            hlog(HLOG_DEBUG, "cmdline exists for %s. Checking to see if it"
                 " is the correct process", mPidFilePath.c_str());

            FString cmdline = mFileSystem->FileGetContents(procCmdLine);
            size_t pos = cmdline.find_first_of('\0');
            if (pos != NOPOS)
                cmdline = cmdline.Left(pos);

            FString basename = mFileSystem->Basename(mExecutableName);
            // test if the basename of the current process and the
            // process that has the pid are the same.
            if (cmdline.length() >= basename.length()
                    && basename.Compare(cmdline.Right(basename.length())) == 0)
            {
                hlog(HLOG_DEBUG, "Process is correct '%s'", basename.c_str());
                return true;
            }
        }
        hlog(HLOG_DEBUG, "/proc/pid/cmdline details do match our process.");
        return false;
    }
    else
    {
        // PID file does not exist
        hlog(HLOG_DEBUG, "%s does not exist.", mPidFilePath.c_str());
        return false;
    }
}
