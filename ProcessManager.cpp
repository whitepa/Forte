// ProcessManager.cpp
#include "ProcessManager.h"
#include "ProcessHandler.h"
#include "AutoMutex.h"
#include "Exception.h"
#include "LogManager.h"
#include "LogTimer.h"
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
    
    return ph;
}

void Forte::ProcessManager::RunProcess(boost::shared_ptr<ProcessHandler> ph)
{
	WaitForInitialize();
	pid_t child_pid = ph->Run();
	processHandlers[child_pid] = ph;
}

void * Forte::ProcessManager::run(void)
{
	FTRACE;
	int child_status = 0;
	hlog(HLOG_DEBUG, "Starting process manager loop");

	mThreadName.Format("processmanager-%u", GetThreadID());
	while (!IsShuttingDown()) {
		if(processHandlers.size()) {
			pid_t tpid;
			hlog(HLOG_DEBUG, "Waiting on something");
			
			tpid = wait(&child_status);
			
			if(tpid == -1) {
				hlog(HLOG_DEBUG, "Error waiting!");
			} else {
			
				// check to see which of our threads this belongs
				ProcessHandlerMap::iterator it;
				it = processHandlers.find(tpid);
				if(it->second) {
					hlog(HLOG_DEBUG, "We have found our man");
					it->second->SetIsRunning(false);
				} else {
					hlog(HLOG_DEBUG, "Who is this? %u", tpid);
					
				}
			}
		}
	}
	return NULL;
}
