#include "Forte.h"

PidFile::PidFile(const char *path) :
    mPath(path)
{
    
    // does the pidfile exist already?
    // TODO: posix advisory lock if so, avoid race condition when deleting a stale pidfile
    FString pidstr;
    try
    {
        FString::LoadFile(path, 0x100, pidstr);
        int oldpid = pidstr.asInteger();
        if (oldpid != getpid())
        {
            FString procfile(FStringFC(), "/proc/%u/cmdline", oldpid);
            FString cmdline;
            try
            {
                FString::LoadFile(procfile, 0x100, cmdline);
                // loaded a cmdline successfully from running process
                hlog(HLOG_DEBUG, "loaded cmdline from running process at PID %u", oldpid);
                throw EAlreadyRunning();
            }
            catch (EFStringLoadFile &e)
            {
                // pidfile does exist, but no associated running process
                hlog(HLOG_DEBUG, "no process currently at PID %u; unlinking pidfile", oldpid);
                unlink(path);
            }
        }
        else // PID == PID
        {
            // stale pid file
            hlog(HLOG_DEBUG, "unlinking stale pidfile %u", oldpid);
            unlink(path);
        }
    }
    catch (EFStringLoadFile &e)
    {
        hlog(HLOG_DEBUG, "no pidfile; will create");
        // pidfile doesn't exist, no problem.
    }
    // create the pidfile (using O_EXCL to avoid race condition --
    // must be local filesystem)
    int fd = open(path, O_CREAT | O_EXCL | O_RDWR, 
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); // 0755 perms
    if (fd < 0)
        throw EPidfileOpenFailed(FStringFC(), "%s", strerror(errno));
    FILE *file = fdopen(fd, "w");
    if (file == NULL)
        throw EPidfileOpenFailed(FStringFC(), "%s", strerror(errno));        
    fprintf(file, "%u\n", getpid());
    fclose(file);
}


PidFile::~PidFile()
{
    // delete the pidfile
    hlog(HLOG_DEBUG, "deleting pidfile");
    unlink(mPath);
}
