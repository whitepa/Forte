// ProcessManager.cpp
#include "ProcessManager.h"
#include "ProcessHandle.h"
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


Forte::ProcessManager::ProcessManager() 
{
	initialized();
}

Forte::ProcessManager::~ProcessManager() 
{

    // we must deal with anyone potentially Wait()'ing on a ProcessHandle to finish
    // running in another thread

    RunningProcessHandleMap::iterator end = runningProcessHandles.end();
    for(RunningProcessHandleMap::iterator it = runningProcessHandles.begin(); it != end; ++it)
    {
        // double check that we have a valid process handle
        if(it->second) 
        {
            boost::shared_ptr<ProcessHandle> ph = it->second;
            ph->SetProcessTerminationType(ProcessNotTerminated);
            ph->NotifyWaiters();
        }

    }
    runningProcessHandles.clear();

    
}

boost::shared_ptr<ProcessHandle> Forte::ProcessManager::CreateProcess(const FString &command,
                                                                      const FString &currentWorkingDirectory,
                                                                      const FString &outputFilename,
                                                                      const FString &inputFilename,
                                                                      const StrStrMap *environment)
{
    boost::shared_ptr<ProcessHandle> ph(new ProcessHandle(command, 
                                                          currentWorkingDirectory, 
                                                          outputFilename,
                                                          inputFilename,
                                                          environment));
	ph->SetProcessManager(this);
	processHandles[ph->GetGUID()] = ph;
    
    return ph;
}

void Forte::ProcessManager::RunProcess(const FString &guid)
{
	AutoUnlockMutex lock(mLock);

	ProcessHandleMap::iterator it = processHandles.find(guid);
	if(it != processHandles.end()) {
        boost::shared_ptr<ProcessHandle> ph = it->second;
        pid_t childPid = fork();
        if(childPid < 0) 
        {
            hlog(HLOG_ERR, "unable to fork child process");
            throw EProcessManagerUnableToFork();
        }
        else if(childPid == 0) 
        {
            // child
            ph->RunChild();
        } 
        else 
        {
            // parent
            ph->RunParent(childPid);
        }
		hlog(HLOG_DEBUG, "Running process (%d) %s", childPid, guid.c_str());
		runningProcessHandles[childPid] = ph;
		Notify();
	}
}

void Forte::ProcessManager::AbandonProcess(const FString &guid)
{
	AutoUnlockMutex lock(mLock);
	ProcessHandleMap::iterator it = processHandles.find(guid);
	if(it != processHandles.end()) {
		RunningProcessHandleMap::iterator rit = runningProcessHandles.find(it->second->GetChildPID());
		if(rit != runningProcessHandles.end()) {
            hlog(HLOG_DEBUG, "abandoning ProcessHandle %s", guid.c_str());
			runningProcessHandles.erase(rit);
		} else {
            hlog(HLOG_ERR, "asked to abandon a non-running process ProcessHandle %s", guid.c_str());
        }
		processHandles.erase(it);
	} else {
        hlog(HLOG_ERR, "asked to abandon an unknown ProcessHandle %s", guid.c_str());
    }
	
}

void* Forte::ProcessManager::run(void)
{
	hlog(HLOG_DEBUG, "Starting ProcessManager runloop");
	AutoUnlockMutex lock(mLock);
	mThreadName.Format("processmanager-%u", GetThreadID());
	while (!IsShuttingDown()) {
		if(runningProcessHandles.size()) {
			pid_t tpid;
			int child_status = 0;
			{
				AutoLockMutex unlock(mLock);
				// unlock mutex here so other threads can add processes while we are waiting
				tpid = wait(&child_status);
			}
			if(tpid == -1) {
                if(errno == ECHILD) {
                    hlog(HLOG_WARN, "wait() error: no unwaited-for children");
                } else if(errno == EINTR) {
                    hlog(HLOG_ERR, "wait() error: unblocked signal or SIGCHILD caught");
                } else if(errno == EINVAL) {
                    // NOTE: we shouldn't receive this error unless we switch to waitpid
                    // but I just kept it here for completeness
                    hlog(HLOG_CRIT, "wait() error: invalid options parameter");
                } else {
                    hlog(HLOG_ERR, "wait() error: unknown error");
                }
			} else {
							
				// check to see which of our threads this belongs
				RunningProcessHandleMap::iterator it;
				it = runningProcessHandles.find(tpid);
				if(it != runningProcessHandles.end() && it->second) {
					boost::shared_ptr<ProcessHandle> ph = it->second;
					// how did the child end?
                    unsigned int statusCode = 0;
					if (WIFEXITED(child_status)) {
                        statusCode = WEXITSTATUS(child_status);
						ph->SetStatusCode(statusCode);
						ph->SetProcessTerminationType(ProcessExited);
						hlog(HLOG_DEBUG, "child exited (status %d)", statusCode);
					} else if (WIFSIGNALED(child_status)) {
                        statusCode = WTERMSIG(child_status);
						ph->SetStatusCode(statusCode);
						ph->SetProcessTerminationType(ProcessKilled);
						hlog(HLOG_DEBUG, "child killed (signal %d)", statusCode);
					} else if (WIFSTOPPED(child_status)) {
                        statusCode = WSTOPSIG(child_status);
						ph->SetStatusCode(statusCode);
						ph->SetProcessTerminationType(ProcessStopped);
						hlog(HLOG_DEBUG, "child stopped (signal %d)", statusCode);
					} else {
						ph->SetStatusCode(child_status);
						ph->SetProcessTerminationType(ProcessUnknownTermination);
						hlog(HLOG_ERR, "unknown child exit status (0x%x)", child_status);
					}
					
					FString guid = ph->GetGUID();
					runningProcessHandles.erase(it);
					ProcessHandleMap::iterator oit = processHandles.find(guid);
					processHandles.erase(oit);

					ph->SetIsRunning(false);
					ProcessHandle::ProcessCompleteCallback callback = ph->GetProcessCompleteCallback();
					if(!callback.empty()) {
						callback(ph);
					}
					
				} else {
					hlog(HLOG_ERR, "wait() returned a process id we don't know about (%u)", tpid);
				}
			}
		} else {
			// wait here for a process to be added
			// unlock mutex
			{
				AutoLockMutex unlock(mLock);
				interruptibleSleep(Timespec::FromSeconds(60));
				continue;
			}
		}
	}
    hlog(HLOG_DEBUG, "Ending ProcessManager runloop");
	return NULL;
}
