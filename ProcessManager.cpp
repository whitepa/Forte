// ProcessManager.cpp
#include "ProcessManager.h"
#include "Process.h"
#include "AutoMutex.h"
#include "Exception.h"
#include "LogManager.h"
#include "LogTimer.h"
#include "Timer.h"
#include "ServerMain.h"
#include "Util.h"
#include "FTrace.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <csignal>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

using namespace Forte;

const int Forte::ProcessManager::MAX_RUNNING_PROCS = 128;
const int Forte::ProcessManager::PDU_BUFFER_SIZE = 4096;

Forte::ProcessManager::ProcessManager() :
    mProcmonPath("/usr/sbin/procmon")
{
    char *procmon = getenv("FORTE_PROCMON");
    if (procmon)
    {
        hlog(HLOG_INFO, "FORTE_PROCMON is set; using alternate procmon at path '%s'", procmon);
        mProcmonPath.assign(procmon);
    }
    initialized();
}

Forte::ProcessManager::~ProcessManager() 
{
    // \TODO wake up waiters, they should throw. This wont happen
    // automatically because the callers will be sharing ownership of
    // the Process objects.

    // \TODO abandon processes

}

boost::shared_ptr<Process> 
Forte::ProcessManager::CreateProcess(const FString &command,
                                     const FString &currentWorkingDirectory,
                                     const FString &outputFilename,
                                     const FString &inputFilename,
                                     const StrStrMap *environment)
{
    FString guid;
    mGUIDGenerator.GenerateGUID(guid);
    boost::shared_ptr<Process> ph(
        new Process(GetPtr(),
                    GetProcmonPath(),
                    guid,
                    command,
                    currentWorkingDirectory,
                    outputFilename,
                    inputFilename,
                    environment));
    mProcesses[guid] = ph;
    ph->startMonitor();
    return ph;
}

void Forte::ProcessManager::abandonProcess(const FString &guid)
{
    mProcesses.erase(guid);
    // OLD
    // \TODO abandoning a process must prevent a zombie process from
    // forming when we fail to call wait() on its PID when it dies.
    // This needs to be re-worked to prevent zombies.
    // AutoUnlockMutex lock(mLock);
    // ProcessMap::iterator it = processHandles.find(guid);
    // if(it != processHandles.end()) {
    //     RunningProcessMap::iterator rit = runningProcesss.find(it->second->GetChildPID());
    //     if(rit != runningProcesss.end()) {
    //         hlog(HLOG_DEBUG, "abandoning Process %s", guid.c_str());
    //         runningProcesss.erase(rit);
    //     } else {
    //         hlog(HLOG_ERR, "asked to abandon a non-running process Process %s", guid.c_str());
    //     }
    //     processHandles.erase(it);
    // } else {
    //     hlog(HLOG_ERR, "asked to abandon an unknown Process %s", guid.c_str());
    // }
}

void Forte::ProcessManager::addPeer(int fd)
{
    hlog(HLOG_DEBUG, "adding ProcessMonitor peer on fd %d", fd);
    mPeerSet.PeerCreate(fd);
}

void* Forte::ProcessManager::run(void)
{
    // OLD
    // mThreadName.Format("processmanager-%u", GetThreadID());
    // hlog(HLOG_DEBUG, "Starting ProcessManager runloop");
    // AutoUnlockMutex lock(mLock);    
    // struct pollfd fds[MAX_RUNNING_PROCS];
    // int nfds = 0;
    // int pollResult;
    // int timeout = 100; // TIMEOUT is 100ms

    // // allocate pdu buffer on heap:
    // shared_array<char> bufferPtr(new char[PDU_BUFFER_SIZE]);
    // char *buffer = bufferPtr.get();

    // while (!IsShuttingDown())
    // {
    //     if (nfds == 0)
    //     {
    //         AutoLockMutex unlock(mLock);
    //         interruptibleSleep(Timespec::FromSeconds(60));
    //         continue;
    //     }
    //     pollResult = poll(fds, nfds, timeout);
    //     if (pollResult < 0)
    //     {
    //         hlog(HLOG_ERR, "poll(): %s", strerror(errno));
    //         AutoLockMutex unlock(mLock);
    //         usleep(100000); // forced sleep to avoid tight loops
    //         continue;
    //     }
    //     else if (pollResult > 0)
    //     {
    //         // some fds are ready
    //         for (int i = 0; i < nfds; ++i)
    //         {
    //             bool error = false;
    //             hlog(HLOG_DEBUG4,"fds[%d].fd = %d", i, fds[i].fd);
    //             hlog(HLOG_DEBUG4,"fds[%d].events = 0x%x", i, fds[i].events);
    //             hlog(HLOG_DEBUG4,"fds[%d].revents = 0x%x", i, fds[i].revents);
    //             if (fds[i].revents & POLLIN)
    //             {
    //                 // input is ready
    //                 hlog(HLOG_DEBUG, "incoming message");
    //                 if ((int)(len = recv(fds[i].fd, buffer, PDU_BUFFER_SIZE, 0)) <= 0)
    //                 {
    //                     hlog(HLOG_ERR, "recv failed: %s", strerror(errno));
    //                     error = true;
    //                 }
    //                 else
    //                 {
    //                     hlog(HLOG_DEBUG, "received %llu bytes", (u64) len);
    //                     shared_ptr<PDUPeer> peer(mPeerSet.PeerGet(fds[i].fd));
    //                     peer->DataIn(len, buffer);
    //                     // handle PDU if ready
    //                     PDU pdu;
    //                     while (peer->RecvPDU(pdu))
    //                         processPDU(*peer, pdu);
    //                 }
    //             }
    //             if (error ||
    //                 (fds[i].revents & POLLERR) ||
    //                 (fds[i].revents & POLLHUP) ||
    //                 (fds[i].revents & POLLRDHUP))
    //             {
    //                 // disconnected, or needs a disconnect
    //                 hlog(HLOG_WARN, "fd %d disconnected", fds[i].fd);
    //                 // update peer object as disconnected, clear buffer
    //                 mPeerSet.PeerDisconnected(fds[i].fd);
    //                 // close and remove socket from pollfds
    //                 close(fds[i].fd);
    //                 for (int j = i; j < nfds - 1; j++)
    //                 {
    //                     fds[j].fd = fds[j+1].fd;
    //                     fds[j].events = fds[j+1].events;
    //                     fds[j].revents = fds[j+1].revents;
    //                 }
    //                 nfds--;
    //                 i--;
    //                 error = false;
    //             }
                
    //         }
    //     }
    // }

    // hlog(HLOG_DEBUG, "Ending ProcessManager runloop");
    // return NULL;
}




        // if(runningProcessHandles.size()) {
        //     pid_t tpid;
        //     int child_status = 0;
        //     {
        //         AutoLockMutex unlock(mLock);
        //         // unlock mutex here so other threads can add processes while we are waiting
        //         tpid = wait(&child_status);
        //     }
        //     if(tpid == -1) {
        //         if(errno == ECHILD) {
        //             hlog(HLOG_WARN, "wait() error: no unwaited-for children");
        //         } else if(errno == EINTR) {
        //             hlog(HLOG_ERR, "wait() error: unblocked signal or SIGCHILD caught");
        //         } else if(errno == EINVAL) {
        //             // NOTE: we shouldn't receive this error unless we switch to waitpid
        //             // but I just kept it here for completeness
        //             hlog(HLOG_CRIT, "wait() error: invalid options parameter");
        //         } else {
        //             hlog(HLOG_ERR, "wait() error: unknown error");
        //         }
        //     } else {
        //         // check to see which of our threads this belongs
        //         RunningProcessHandleMap::iterator it;
        //         it = runningProcessHandles.find(tpid);
        //         if(it != runningProcessHandles.end() && it->second) {
        //             boost::shared_ptr<ProcessHandle> ph = it->second;
        //             // how did the child end?
        //             unsigned int statusCode = 0;
        //             if (WIFEXITED(child_status)) {
        //                 statusCode = WEXITSTATUS(child_status);
        //                 ph->SetStatusCode(statusCode);
        //                 ph->SetProcessTerminationType(ProcessExited);
        //                 hlog(HLOG_DEBUG, "child exited (status %d)", statusCode);
        //             } else if (WIFSIGNALED(child_status)) {
        //                 statusCode = WTERMSIG(child_status);
        //                 ph->SetStatusCode(statusCode);
        //                 ph->SetProcessTerminationType(ProcessKilled);
        //                 hlog(HLOG_DEBUG, "child killed (signal %d)", statusCode);
        //             } else if (WIFSTOPPED(child_status)) {
        //                 statusCode = WSTOPSIG(child_status);
        //                 ph->SetStatusCode(statusCode);
        //                 ph->SetProcessTerminationType(ProcessStopped);
        //                 hlog(HLOG_DEBUG, "child stopped (signal %d)", statusCode);
        //             } else {
        //                 ph->SetStatusCode(child_status);
        //                 ph->SetProcessTerminationType(ProcessUnknownTermination);
        //                 hlog(HLOG_ERR, "unknown child exit status (0x%x)", child_status);
        //             }
					
        //             FString guid = ph->GetGUID();
        //             runningProcessHandles.erase(it);
        //             ProcessHandleMap::iterator oit = processHandles.find(guid);
        //             processHandles.erase(oit);

        //             ph->SetIsRunning(false);
        //             ProcessHandle::ProcessCompleteCallback callback = ph->GetProcessCompleteCallback();
        //             if(!callback.empty()) {
        //                 callback(ph);
        //             }
					
        //         } else {
        //             hlog(HLOG_ERR, "wait() returned a process id we don't know about (%u)", tpid);
        //         }
        //     }
