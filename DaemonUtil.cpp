#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "DaemonUtil.h"
#include "LogManager.h"

using namespace Forte;

void DaemonUtil::Daemonize(void)
{
    if (!ForkDaemon())
        exit(0);
}

bool DaemonUtil::ForkDaemon(void)
{
    // TODO: figure out how to handle failures of these system calls
    // in the child(ren), and propogate the error back to the parent.

    pid_t pid;

    // Fork off the parent process
    pid = ForkSafely();
    if (pid < 0) {
        throw EDaemonForkFailedException();
    }
    // If we got a good PID, then we are the parent.
    else if (pid > 0) {
        hlog(HLOG_DEBUG, "fork succeeded, child PID is %d", pid);
        return false;
    }

    // At this point we are executing as the child process

    // Change the file mode mask
    umask(0);

    // Create a new SID for the child process
    setsid();

    // Change the current working directory.
    if ((chdir("/")) < 0)
        exit(1);

    // Redirect standard files to /dev/null
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);

    if ((pid = fork()) < 0)
        exit(1);
    else if (pid > 0)
        exit(0); // first child

    // second child
    setsid();

    return true;
}

pid_t DaemonUtil::ForkSafely(void)
{
    try
    {
        AutoUnlockMutex logSingletonLock(LogManager::GetSingletonMutex());
        AutoUnlockMutex logLock(LogManager::GetInstance().GetMutex());
        return fork();
    }
    catch (EEmptyReference &e)
    {
        printf("ForkSafely: EEmptyReference\n");
    }
    return fork();
}
