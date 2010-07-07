// ProcessManager.cpp
#include "ProcessManager.h"
#include "ProcessHandler.h"
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

}

boost::shared_ptr<ProcessHandler> Forte::ProcessManager::CreateProcess(const FString &command,
                                                                       const FString &currentWorkingDirectory,
                                                                       const StrStrMap *environment,
                                                                       const FString &inputFilename)
{
    boost::shared_ptr<ProcessHandler> ph(new ProcessHandler(command, 
                                                            currentWorkingDirectory, 
                                                            environment, 
                                                            inputFilename));
	ph->SetProcessManager(this);
	processHandlers[ph->GetGUID()] = ph;
    
    return ph;
}

void Forte::ProcessManager::RunProcess(const FString &guid)
{
	// do I need this?
	WaitForInitialize();
	
	AutoUnlockMutex lock(mLock);
	ProcessHandlerMap::iterator it = processHandlers.find(guid);
	if(it != processHandlers.end()) {
		runningProcessHandlers[it->second->GetChildPID()] = it->second;
		processHandlers.erase(it);
		Notify();
	}
}

void* Forte::ProcessManager::run(void)
{
	FTRACE;
	int child_status = 0;
	hlog(HLOG_DEBUG, "Starting process manager loop");
	AutoUnlockMutex lock(mLock);
	mThreadName.Format("processmanager-%u", GetThreadID());
	while (!IsShuttingDown()) {
		if(runningProcessHandlers.size()) {
			pid_t tpid;
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
				RunningProcessHandlerMap::iterator it;
				it = runningProcessHandlers.find(tpid);
				if(it->second) {
					hlog(HLOG_DEBUG, "We have found our man");
					it->second->SetIsRunning(false);
					
					// how did the child end?
					if (WIFEXITED(child_status)) {
						it->second->SetStatusCode(WEXITSTATUS(child_status));
						it->second->SetProcessTerminationType(ProcessExited);
						hlog(HLOG_DEBUG, "child exited (status %d)", it->second->GetStatusCode());
					} else if (WIFSIGNALED(child_status)) {
						it->second->SetStatusCode(WTERMSIG(child_status));
						it->second->SetProcessTerminationType(ProcessKilled);
						hlog(HLOG_DEBUG, "child killed (signal %d)", it->second->GetStatusCode());
					} else if (WIFSTOPPED(child_status)) {
						it->second->SetStatusCode(WSTOPSIG(child_status));
						it->second->SetProcessTerminationType(ProcessStopped);
						hlog(HLOG_DEBUG, "child stopped (signal %d)", it->second->GetStatusCode());
					} else {
						it->second->SetStatusCode(child_status);
						it->second->SetProcessTerminationType(ProcessUnknownTermination);
						hlog(HLOG_DEBUG, "unknown child status (0x%x)", it->second->GetStatusCode());
					}
					
					// TODO: find out if this ph has a callback, and then send it
					
					runningProcessHandlers.erase(it);
					
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
