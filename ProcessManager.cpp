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
	AutoLockMutex unlock(mLock);
}

boost::shared_ptr<ProcessHandle> Forte::ProcessManager::CreateProcess(const FString &command,
                                                                      const FString &currentWorkingDirectory,
                                                                      const FString &inputFilename,
                                                                      const FString &outputFilename,
                                                                      const StrStrMap *environment)
{
    boost::shared_ptr<ProcessHandle> ph(new ProcessHandle(command, 
                                                          currentWorkingDirectory, 
                                                          inputFilename,
                                                          outputFilename,
                                                          environment));
	ph->SetProcessManager(this);
	processHandlers[ph->GetGUID()] = ph;
    
    return ph;
}

void Forte::ProcessManager::RunProcess(const FString &guid)
{
	AutoUnlockMutex lock(mLock);
	ProcessHandleMap::iterator it = processHandlers.find(guid);
	if(it != processHandlers.end()) {
		hlog(HLOG_DEBUG, "Running process (%d) %s", it->second->GetChildPID(), guid.c_str());
		runningProcessHandles[it->second->GetChildPID()] = it->second;
		Notify();
	}
}

void Forte::ProcessManager::AbandonProcess(const FString &guid)
{
	AutoUnlockMutex lock(mLock);
	ProcessHandleMap::iterator it = processHandlers.find(guid);
	if(it != processHandlers.end()) {
		RunningProcessHandleMap::iterator rit = runningProcessHandles.find(it->second->GetChildPID());
		if(rit != runningProcessHandles.end()) {
			runningProcessHandles.erase(rit);
		}
		processHandlers.erase(it);
	}
	
}

void* Forte::ProcessManager::run(void)
{
	hlog(HLOG_DEBUG, "Starting process manager loop");
	AutoUnlockMutex lock(mLock);
	mThreadName.Format("processmanager-%u", GetThreadID());
	while (!IsShuttingDown()) {
		if(runningProcessHandles.size()) {
			pid_t tpid;
			int child_status = 0;
			hlog(HLOG_DEBUG, "Waiting on our children to come home");
			
			{
				AutoLockMutex unlock(mLock);
				// unlock mutex here so other threads can add processes while we are waiting
				tpid = wait(&child_status);
			}
			if(tpid == -1) {
				hlog(HLOG_DEBUG, "Error waiting!");
			} else {
							
				// check to see which of our threads this belongs
				RunningProcessHandleMap::iterator it;
				it = runningProcessHandles.find(tpid);
				if(it->second) {
					boost::shared_ptr<ProcessHandle> ph = it->second;
					hlog(HLOG_DEBUG, "We have found our man");
					
					// how did the child end?
					if (WIFEXITED(child_status)) {
						ph->SetStatusCode(WEXITSTATUS(child_status));
						ph->SetProcessTerminationType(ProcessExited);
						hlog(HLOG_DEBUG, "child exited (status %d)", ph->GetStatusCode());
					} else if (WIFSIGNALED(child_status)) {
						ph->SetStatusCode(WTERMSIG(child_status));
						ph->SetProcessTerminationType(ProcessKilled);
						hlog(HLOG_DEBUG, "child killed (signal %d)", ph->GetStatusCode());
					} else if (WIFSTOPPED(child_status)) {
						ph->SetStatusCode(WSTOPSIG(child_status));
						ph->SetProcessTerminationType(ProcessStopped);
						hlog(HLOG_DEBUG, "child stopped (signal %d)", ph->GetStatusCode());
					} else {
						ph->SetStatusCode(child_status);
						ph->SetProcessTerminationType(ProcessUnknownTermination);
						hlog(HLOG_DEBUG, "unknown child status (0x%x)", ph->GetStatusCode());
					}
					
					FString guid = ph->GetGUID();
					runningProcessHandles.erase(it);
					ProcessHandleMap::iterator oit = processHandlers.find(guid);
					processHandlers.erase(oit);

					ph->SetIsRunning(false);
					ProcessHandle::ProcessCompleteCallback callback = ph->GetProcessCompleteCallback();
					if(!callback.empty()) {
						callback(ph);
					}
					
				} else {
					hlog(HLOG_DEBUG, "Who is this? %u", tpid);
					
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
	return NULL;
}
