// ProcessManager.cpp
#include "ProcessManager.h"
#include "ProcessFuture.h"
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

    mPeerSet.SetupEPoll();
    mPeerSet.SetProcessPDUCallback(boost::bind(&ProcessManager::pduCallback, this, _1));
    mPeerSet.SetErrorCallback(boost::bind(&ProcessManager::errorCallback, this, _1));

    initialized();
}

Forte::ProcessManager::~ProcessManager() 
{
    // \TODO wake up waiters, they should throw. This wont happen
    // automatically because the callers will be sharing ownership of
    // the Process objects.

    // \TODO abandon processes
    
    // shut down the thread
    deleting();
}

boost::shared_ptr<ProcessFuture> 
Forte::ProcessManager::CreateProcess(const FString &command,
                                     const FString &currentWorkingDirectory,
                                     const FString &outputFilename,
                                     const FString &inputFilename,
                                     const StrStrMap *environment)
{
    FTRACE;
    boost::shared_ptr<ProcessFuture> ph(
        new ProcessFuture(GetPtr(),
                          GetProcmonPath(),
                          command,
                          currentWorkingDirectory,
                          outputFilename,
                          inputFilename,
                          environment));
    ph->startMonitor();
    {
        AutoUnlockMutex lock(mProcessesLock);
        mProcesses[ph->getManagementFD()] = ph;
    }

    ph->run();
    return ph;
}

boost::shared_ptr<ProcessFuture> 
Forte::ProcessManager::CreateProcessDontRun(const FString &command,
                                            const FString &currentWorkingDirectory,
                                            const FString &outputFilename,
                                            const FString &inputFilename,
                                            const StrStrMap *environment)
{
    FTRACE;
    boost::shared_ptr<ProcessFuture> ph(
        new ProcessFuture(GetPtr(),
                          GetProcmonPath(),
                          command,
                          currentWorkingDirectory,
                          outputFilename,
                          inputFilename,
                          environment));
    ph->startMonitor();
    AutoUnlockMutex lock(mProcessesLock);
    mProcesses[ph->getManagementFD()] = ph;

    return ph;
}

void Forte::ProcessManager::RunProcess(
    boost::shared_ptr<ProcessFuture> ph)
{
    ph->run();
}

void Forte::ProcessManager::abandonProcess(const int fd)
{
    // don't allow the object to be destroyed while the lock is held,
    // as the Process destructor also will attempt to abandon itself
    // here.

    // order of declarations is important!
    // boost::shared_ptr<ProcessFuture> processPtr;
    AutoUnlockMutex lock(mProcessesLock);
    ProcessMap::iterator i = mProcesses.find(fd);
    if (i == mProcesses.end()) return;
    //processPtr = (*i).second.lock();
    mProcesses.erase(fd);
    // lock is unlocked
    // object is destroyed
}

boost::shared_ptr<Forte::PDUPeer> Forte::ProcessManager::addPeer(int fd)
{
    hlog(HLOG_DEBUG, "adding ProcessMonitor peer on fd %d", fd);
    return mPeerSet.PeerCreate(fd);
}

void Forte::ProcessManager::pduCallback(PDUPeer &peer)
{
    FTRACE;
    // route this to the correct Process object
    hlog(HLOG_DEBUG, "Getting fd from peer");
    int fd = peer.GetFD();

    // ordering of the variables p and lock and important
    // p needs to declared first and then lock, so that 
    // they get destructed appropiately
    hlog(HLOG_DEBUG, "Going to obtain processes lock");
    boost::shared_ptr<ProcessFuture> p;
    AutoUnlockMutex lock(mProcessesLock);
    hlog(HLOG_DEBUG, "obtained lock");

    ProcessMap::iterator i;
    if ((i = mProcesses.find(fd)) == mProcesses.end())
        throw EProcessManagerInvalidPeer();

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
        throw EProcessManagerNoSharedPtr();
    }
}

void Forte::ProcessManager::errorCallback(PDUPeer &peer)
{
    FTRACE;
    // route this to the correct Process object
    int fd = peer.GetFD();

    // ordering of the variables p and lock and important
    // p needs to declared first and then lock, so that 
    // they get destructed appropiately
    boost::shared_ptr<ProcessFuture> p;
    AutoUnlockMutex lock(mProcessesLock);
    ProcessMap::iterator i;
    if ((i = mProcesses.find(fd)) == mProcesses.end())
        throw EProcessManagerInvalidPeer();

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
        throw EProcessManagerNoSharedPtr();
    }
}

void* Forte::ProcessManager::run(void)
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
            hlog(HLOG_ERR, "caught exception: %s", e.what());
            sleep(1); // prevent tight loops
        }
    }
    return NULL;
}
