#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "LogManager.h"
#include "PidFile.h"
#include "FTrace.h"
#include "SystemCallUtil.h"

using namespace Forte;

PidFile::PidFile(FileSystemPtr fs, const char *path) :
    mPath(path), mFileSystem(fs), mShouldDeleteWhenObjectDestroyed(false)
{
    
}

void PidFile::Create(unsigned int pid)
{
    FTRACE2("%u", pid);

    mShouldDeleteWhenObjectDestroyed=true;
    if (IsProcessRunning())
    {
        throw EAlreadyRunning();
    }

    if (pid == 0)
    {
        pid = getpid();
    }

    // create the pidfile (using O_EXCL to avoid race condition --
    // must be local filesystem)
    AutoFD fd;
    mFileSystem->FileOpen(fd, mPath, O_CREAT | O_EXCL | O_RDWR, // 0755 perms 
                         S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    mFileSystem->FilePutContents(fd, "%u", pid);

    hlog(HLOG_DEBUG, "Pid file (%s) created with pid %u.", mPath.c_str(), pid);
}

void PidFile::Delete(void)
{
    FTRACE;
    // delete the pidfile
    hlog(HLOG_DEBUG, "deleting pidfile");
    mFileSystem->Unlink(mPath);
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
    FTRACE2("%s", mPath.c_str());

    if (mFileSystem->FileExists(mPath))
    {
        hlog(HLOG_DEBUG, "%s exists.  Checking for process", mPath.c_str());
        unsigned int pid=mFileSystem->FileGetContents(mPath).AsUnsignedInteger();
        FString procCmdLine(FStringFC(), "/proc/%u/cmdline", pid);
        return mFileSystem->FileExists(procCmdLine);
    }
    else
    {
        // PID file does not exist 
        hlog(HLOG_DEBUG, "%s does not exist.", mPath.c_str());
        return false;
    }
}
