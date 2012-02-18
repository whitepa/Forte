// ProcessManagerImpl.cpp
#include "DaemonUtil.h"
#include "ProcessManagerImpl.h"
#include "ProcessFuture.h"
#include "AutoMutex.h"
#include "Exception.h"
#include "LogManager.h"
#include "LogTimer.h"
#include "Timer.h"
#include "ServerMain.h"
#include "Util.h"
#include "FTrace.h"
#include "GUIDGenerator.h"
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

const int Forte::ProcessManagerImpl::MAX_RUNNING_PROCS = 128;
const int Forte::ProcessManagerImpl::PDU_BUFFER_SIZE = 4096;

Forte::ProcessManagerImpl::ProcessManagerImpl() :
    mProcmonPath("/usr/libexec/procmon")
{
    FTRACE;
    setThreadName(FString(FStringFC(), "procmon-%u", GetThreadID()));
    char *procmon = getenv("FORTE_PROCMON");
    if (procmon)
    {
        hlog(HLOG_INFO, "FORTE_PROCMON is set; using alternate procmon at path '%s'", procmon);
        mProcmonPath.assign(procmon);
    }

    mPeerSet.SetupEPoll();
    mPeerSet.SetProcessPDUCallback(boost::bind(&ProcessManagerImpl::pduCallback, this, _1));
    mPeerSet.SetErrorCallback(boost::bind(&ProcessManagerImpl::errorCallback, this, _1));

    initialized();
}

Forte::ProcessManagerImpl::~ProcessManagerImpl()
{
    FTRACE;
    // \TODO wake up waiters, they should throw. This wont happen
    // automatically because the callers will be sharing ownership of
    // the Process objects.

    // \TODO abandon processes

    // shut down the thread
    deleting();
}

boost::shared_ptr<ProcessFuture>
Forte::ProcessManagerImpl::CreateProcess(const FString &command,
                                         const FString &currentWorkingDirectory,
                                         const FString &outputFilename,
                                         const FString &errorFilename,
                                         const FString &inputFilename,
                                         const StrStrMap *environment,
                                         const FString &commandToLog)
{
    FTRACE;
    boost::shared_ptr<ProcessManagerImpl> mgr =
        dynamic_pointer_cast<ProcessManagerImpl>(GetPtr());
    boost::shared_ptr<ProcessFutureImpl> ph(
        new ProcessFutureImpl(mgr,
                              command,
                              currentWorkingDirectory,
                              outputFilename,
                              errorFilename,
                              inputFilename,
                              environment,
                              commandToLog));
    startMonitor(ph);
    {
        AutoUnlockMutex lock(mProcessesLock);
        mProcesses[ph->getManagementFD()] = ph;
    }

    ph->run();
    return ph;
}

boost::shared_ptr<ProcessFuture>
Forte::ProcessManagerImpl::CreateProcessDontRun(const FString &command,
                                                const FString &currentWorkingDirectory,
                                                const FString &outputFilename,
                                                const FString &errorFilename,
                                                const FString &inputFilename,
                                                const StrStrMap *environment,
                                                const FString &commandToLog)
{
    FTRACE;
    boost::shared_ptr<ProcessManagerImpl> mgr =
        dynamic_pointer_cast<ProcessManagerImpl>(GetPtr());
    boost::shared_ptr<ProcessFutureImpl> ph(
        new ProcessFutureImpl(mgr,
                              command,
                              currentWorkingDirectory,
                              outputFilename,
                              errorFilename,
                              inputFilename,
                              environment,
                              commandToLog));
    startMonitor(ph);
    AutoUnlockMutex lock(mProcessesLock);
    mProcesses[ph->getManagementFD()] = ph;

    return ph;
}

void Forte::ProcessManagerImpl::RunProcess(
    boost::shared_ptr<ProcessFuture> ph)
{
    boost::shared_ptr<ProcessFutureImpl> pfi =
        dynamic_pointer_cast<ProcessFutureImpl>(ph);
    pfi->run();
}

void Forte::ProcessManagerImpl::abandonProcess(const boost::shared_ptr<Forte::PDUPeer> &peer)
{
    // don't allow the object to be destroyed while the lock is held,
    // as the Process destructor also will attempt to abandon itself
    // here.

    // order of declarations is important!
    // boost::shared_ptr<ProcessFuture> processPtr;
    {
        AutoUnlockMutex lock(mProcessesLock);
        ProcessMap::iterator i = mProcesses.find(peer->GetFD());
        if (i == mProcesses.end()) return;
        //processPtr = (*i).second.lock();
        mProcesses.erase(i);
        // lock is unlocked
        // object is destroyed
    }
    mPeerSet.PeerDelete(peer);
}

void Forte::ProcessManagerImpl::startMonitor(
    boost::shared_ptr<Forte::ProcessFutureImpl> ph)
{
    int fds[2];

    try
    {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
            throw_exception(EProcessManagerUnableToCreateSocket(FStringFC(), "%s", strerror(errno)));
        AutoFD parentfd(fds[0]);
        AutoFD childfd(fds[1]);

        pid_t childPid = DaemonUtil::ForkSafely();
        if(childPid < 0)
            throw_exception(EProcessManagerUnableToFork(FStringFC(), "%s", strerror(errno)));
        else if(childPid == 0)
        {
            //fprintf(stderr, "procmon child, childfd=%d\n", childfd.GetFD());
            // child
            // close all file descriptors, except the PDU channel
            while (close(parentfd) == -1 && errno == EINTR);
            for (int n = 3; n < 1024; ++n)
                if (n != childfd)
                    while (close(n) == -1 && errno == EINTR);
            // clear sig mask
            sigset_t set;
            sigemptyset(&set);
            pthread_sigmask(SIG_SETMASK, &set, NULL);

            // create a new process group / session
            // (redirects 0,1,2 to /dev/null, forks, parent calls _exit)
            if (daemon(1, 0) == -1)
            {
                fprintf(stderr, "procmon child, daemon() failed: %d %s\n", errno, strerror(errno));
                exit(-1);
            }
            setsid();
            FString childfdStr(childfd);
            char **vargs = new char* [3];
            vargs[0] = const_cast<char *>("(procmon)"); // \TODO include the name of the monitored process
            vargs[1] = const_cast<char *>(childfdStr.c_str());
            vargs[2] = 0;
//        fprintf(stderr, "procmon child, exec '%s' '%s'\n", mProcmonPath.c_str(), vargs[1]);
            execv(mProcmonPath, vargs);
            fprintf(stderr, "procmon child, exec() failed: %d %s\n", errno, strerror(errno));
            exit(-1);
        }
        else
        {
            // parent
            childfd.Close();
            // add a PDUPeer to the PeerSet owned by the ProcessManager
            //shared_ptr<ProcessManager> pm(mProcessManagerPtr.lock());
            ph->mManagementChannel = addPeer(parentfd);
            parentfd.Release();

            // wait for process to deamonise
            // if we don't do this we will leave
            // behind zombie processes
            int childStatus;
            int retCode;
            // while this pid still exists to be waited on
            while ((retCode = waitpid(childPid, &childStatus, 0)) == 0
                   || (retCode == -1 && errno == EINTR))
            {
                usleep(100000);
            }

            if (retCode != childPid && errno != ECHILD)
            {
                hlog_and_throw(HLOG_ERR,
                               EProcessManagerUnableToCreateProcmon(
                                   FStringFC(),
                                   "err waiting on child pid: %d, %d (%s)",
                                   childPid, errno, strerror(errno)));
            }
            else if (WIFEXITED(childStatus) && WEXITSTATUS(childStatus) != 0)
            {
                hlog_and_throw(HLOG_ERR,
                               EProcessManagerUnableToCreateProcmon(
                                   FStringFC(),
                                   "procmon child failed with status %d",
                                   WEXITSTATUS(childStatus)));
            }
        }
    }
    catch (EProcessManagerUnableToCreateSocket &e)
    {
        hlog(HLOG_ERR, "Unable to create socket for process monitor: '%s'", e.what());
        throw;
    }
    catch (EProcessManagerUnableToFork &e)
    {
        hlog(HLOG_ERR, "Unable to fork for process monitor: '%s'", e.what());
        throw;
    }
    catch (...)
    {
        hlog(HLOG_ERR, "Unknown error starting process monitor");
        throw;
    }
}

boost::shared_ptr<Forte::PDUPeer> Forte::ProcessManagerImpl::addPeer(int fd)
{
    hlog(HLOG_DEBUG, "adding ProcessMonitor peer on fd %d", fd);
    return mPeerSet.PeerCreate(fd);
}

int ProcessManagerImpl::CreateProcessAndGetResult(const FString& command,
                                                  FString& output,
                                                  const Timespec &timeout,
                                                  const FString &inputFilename,
                                                  const StrStrMap *environment,
                                                  const FString &commandToLog)
{
    FTRACE;
    FString randomSuffix;
    GUIDGenerator guidGen;
    guidGen.GenerateGUID(randomSuffix);
    FString outputFilename(FStringFC(), "/tmp/sc_commandoutput_%s.tmp",
                          randomSuffix.c_str());
    FString errorFilename(FStringFC(), "/tmp/sc_commandoutput_error_%s.tmp",
                          randomSuffix.c_str());

    hlog(HLOG_DEBUG, "command = %s, timeout=%ld, output=%s",
         command.c_str(), timeout.AsSeconds(), outputFilename.c_str());
    boost::shared_ptr<ProcessFuture> future =
        CreateProcess(command, "/", outputFilename,
                      errorFilename, inputFilename, environment, commandToLog);

    try
    {
        future->GetResultTimed(timeout);
        output = future->GetOutputString();
        hlog(HLOG_DEBUG, "output = %s", output.c_str());
        int statusCode = future->GetStatusCode();
        hlog(HLOG_DEBUG, "status = %d", statusCode);

        if (unlink(outputFilename.c_str()))
        {
            hlog(HLOG_WARN, "Failed to unlink temporary file %s", strerror(errno));
        }
        if (unlink(errorFilename.c_str()))
        {
            hlog(HLOG_WARN, "Failed to unlink temporary file %s", strerror(errno));
        }
        return statusCode;
    }
    catch (EProcessFutureTerminatedWithNonZeroStatus &e)
    {
        // in this case we can still get the
        // output
        output = future->GetErrorString();
        hlog(HLOG_DEBUG, "output = %s", output.c_str());

        if (unlink(outputFilename.c_str()))
        {
            hlog(HLOG_WARN, "Failed to unlink temporary file %s", strerror(errno));
        }
        if (unlink(errorFilename.c_str()))
        {
            hlog(HLOG_WARN, "Failed to unlink temporary file %s", strerror(errno));
        }
        throw;
    }
    catch (Exception& e)
    {
        hlog(HLOG_DEBUG, "exception throw : %s", e.what());
        if (unlink(outputFilename.c_str()))
        {
            hlog(HLOG_WARN, "Failed to unlink temporary file %s", strerror(errno));
        }
        if (unlink(errorFilename.c_str()))
        {
            hlog(HLOG_WARN, "Failed to unlink temporary file %s", strerror(errno));
        }
        throw;
    }
}


void Forte::ProcessManagerImpl::pduCallback(PDUPeer &peer)
{
    FTRACE;
    // route this to the correct Process object
    hlog(HLOG_DEBUG, "Getting fd from peer");
    int fd = peer.GetFD();

    // ordering of the variables p and lock and important
    // p needs to declared first and then lock, so that
    // they get destructed appropiately
    hlog(HLOG_DEBUG, "Going to obtain processes lock");
    boost::shared_ptr<ProcessFutureImpl> p;
    AutoUnlockMutex lock(mProcessesLock);
    hlog(HLOG_DEBUG, "obtained lock");

    ProcessMap::iterator i;
    if ((i = mProcesses.find(fd)) == mProcesses.end())
        throw_exception(EProcessManagerInvalidPeer());

    hlog(HLOG_DEBUG, "Going to get shared pointer for fd %d", fd);
    p = (*i).second.lock();
    if (p.get() != NULL)
    {
        hlog(HLOG_DEBUG, "Got shared pointer for fd %d", fd);
        p->handlePDU(peer);
    }
    else
    {
        // could not obtain shared_ptr
        // from weak ptr. Probably the owner of ProcessFuture
        // handle deleted it but did not call abandon in the
        // ProcessManager. This should not happen since, the
        // destructor of ProcessFuture should abandon the process
        // thereby deleting this process from the ProcessManager
        // map
        hlog(HLOG_ERROR, "Unable to get shared pointer for fd %d", fd);
        throw_exception(EProcessManagerNoSharedPtr());
    }
}

void Forte::ProcessManagerImpl::errorCallback(PDUPeer &peer)
{
    FTRACE;
    // route this to the correct Process object
    int fd = peer.GetFD();

    // ordering of the variables p and lock and important
    // p needs to declared first and then lock, so that
    // they get destructed appropiately
    boost::shared_ptr<ProcessFutureImpl> p;
    AutoUnlockMutex lock(mProcessesLock);
    ProcessMap::iterator i;
    if ((i = mProcesses.find(fd)) == mProcesses.end())
        throw_exception(EProcessManagerInvalidPeer());

    p = (*i).second.lock();
    if (p.get() != NULL)
    {
        p->handleError(peer);
    }
    else
    {
        // could not obtain shared_ptr
        // from weak ptr. Probably the owner of ProcessFuture
        // handle deleted it but did not call abandon in the
        // ProcessManager. This should not happen since, the
        // destructor of ProcessFuture should abandon the process
        // thereby deleting this process from the ProcessManager
        // map
        throw_exception(EProcessManagerNoSharedPtr());
    }
}

void* Forte::ProcessManagerImpl::run(void)
{
    while (!mThreadShutdown)
    {
        try
        {
            // 100ms poll timeout
            mPeerSet.Poll(100);
        }
        catch (Exception &e)
        {
            hlog(HLOG_ERR, "process manager thread caught exception: %s", e.what());
            interruptibleSleep(Forte::Timespec::FromSeconds(1)); // prevent tight loops
        }
    }
    return NULL;
}

bool Forte::ProcessManagerImpl::IsProcessMapEmpty(void)
{
    AutoUnlockMutex lock(mProcessesLock);
    return (mProcesses.size() == 0);
}

