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

boost::shared_ptr<Process> 
Forte::ProcessManager::CreateProcess(const FString &command,
                                     const FString &currentWorkingDirectory,
                                     const FString &outputFilename,
                                     const FString &inputFilename,
                                     const StrStrMap *environment)
{
    FTRACE;
    boost::shared_ptr<Process> ph(
        new Process(GetPtr(),
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

void Forte::ProcessManager::abandonProcess(const int fd)
{
    // don't allow the object to be destroyed while the lock is held,
    // as the Process destructor also will attempt to abandon itself
    // here.

    // order of declarations is important!
    boost::shared_ptr<Process> processPtr;
    AutoUnlockMutex lock(mProcessesLock);
    ProcessMap::iterator i = mProcesses.find(fd);
    if (i == mProcesses.end()) return;
    processPtr = (*i).second;
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
    int fd = peer.GetFD();

    AutoUnlockMutex lock(mProcessesLock);
    ProcessMap::iterator i;
    if ((i = mProcesses.find(fd)) == mProcesses.end())
        throw EProcessManagerInvalidPeer();
    Process &p(*((*i).second));
    p.handlePDU(peer);
}

void Forte::ProcessManager::errorCallback(PDUPeer &peer)
{
    FTRACE;
    // route this to the correct Process object
    int fd = peer.GetFD();

    AutoUnlockMutex lock(mProcessesLock);
    ProcessMap::iterator i;
    if ((i = mProcesses.find(fd)) == mProcesses.end())
        throw EProcessManagerInvalidPeer();
    Process &p(*((*i).second));
    p.handleError(peer);
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
