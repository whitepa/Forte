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

        FString basename = mFileSystem->Basename(mExecutableName);

        try
        {
            FString procExe(FStringFC(), "/proc/%u/exe", pid);
            if (mFileSystem->FileExists(procExe))
            {
                hlog(HLOG_DEBUG, "exe exists for %s. Checking to see if it"
                     " is the correct process", mPidFilePath.c_str());

                FString exe = mFileSystem->ResolveSymLink(procExe);

                // test if the basename of the current process and the
                // process that has the pid are the same.
                if (exe.length() >= basename.length()
                    && basename.Compare(exe.Right(basename.length())) == 0)
                {
                    hlog(HLOG_DEBUG, "Process is correct '%s'", basename.c_str());
                    return true;
                }
                hlog(HLOG_DEBUG, "%s details do not match our process.", procExe.c_str());
            }
        }
        catch (const EFileSystemResolveSymLink &e)
        {
            hlog(HLOG_DEBUG,
                 "Absorbing exception thrown while resolving symlink /proc/%u/exe: %s",
                 pid, e.what());
        }

        try
        {
            FString procCmdLine(FStringFC(), "/proc/%u/cmdline", pid);
            if (mFileSystem->FileExists(procCmdLine))
            {
                hlog(HLOG_DEBUG, "cmdline exists for %s. Checking to see if it"
                     " is the correct process", mPidFilePath.c_str());

                FString cmdline = mFileSystem->FileGetContents(procCmdLine);
                size_t pos = cmdline.find_first_of('\0');
                if (pos != NOPOS)
                    cmdline = cmdline.Left(pos);

                // test if the basename of the current process and the
                // process that has the pid are the same.
                if (cmdline.length() >= basename.length()
                    && basename.Compare(cmdline.Right(basename.length())) == 0)
                {
                    hlog(HLOG_DEBUG, "Process is correct '%s'", basename.c_str());
                    return true;
                }
                hlog(HLOG_DEBUG,
                     "%s details do not match our process either.", procCmdLine.c_str());
            }
        }
        catch (const std::exception &e)
        {
            hlog(HLOG_DEBUG,
                 "Absorbing exception thrown while parsing /proc/%u/cmdline: %s", pid, e.what());
        }

        try
        {
            FString procStat(FStringFC(), "/proc/%u/stat", pid);
            if (mFileSystem->FileExists(procStat))
            {
                hlog(HLOG_DEBUG, "stat exists for %s. Checking to see if it"
                     " is the correct process", mPidFilePath.c_str());

                FStringVector stats;
                mFileSystem->FileGetContents(procStat).RegexMatch("\\((.*?)\\)", stats);

                if (stats.at(0).length() >= basename.length()
                    && basename.Compare(stats.at(0)) == 0)
                {
                    hlog(HLOG_DEBUG, "Process is correct '%s'", basename.c_str());
                    return true;
                }
                hlog(HLOG_DEBUG,
                     "%s details do not match our process either.", procStat.c_str());
            }
        }
        catch (const std::exception &e)
        {
            hlog(HLOG_WARN,
                 "Caught an exception parsing /proc/%u/stat: %s", pid, e.what());
            throw;
        }
    }
    else
    {
        // PID file does not exist
        hlog(HLOG_DEBUG, "%s does not exist.", mPidFilePath.c_str());
    }
    return false;
}
