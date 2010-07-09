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
	processHandles[ph->GetGUID()] = ph;
    
    return ph;
}

void Forte::ProcessManager::RunProcess(const FString &guid)
{
	AutoUnlockMutex lock(mLock);
	ProcessHandleMap::iterator it = processHandles.find(guid);
	if(it != processHandles.end()) {
		hlog(HLOG_DEBUG, "Running process (%d) %s", it->second->GetChildPID(), guid.c_str());
		runningProcessHandles[it->second->GetChildPID()] = it->second;
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
				if(it->second) {
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
	return NULL;
}
