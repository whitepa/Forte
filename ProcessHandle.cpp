// ProcessHandle.cpp

#include "ProcessHandle.h"
#include "ProcessManager.h"
#include "AutoMutex.h"
#include "Exception.h"
#include "LogManager.h"
#include "LogTimer.h"
#include "ServerMain.h"
#include "Util.h"
#include "FTrace.h"
#include "GUID.h"
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
#include <boost/algorithm/string.hpp>


using namespace Forte;

Forte::ProcessHandle::ProcessHandle(const FString &command, 
                                    const FString &currentWorkingDirectory, 
                                    const FString &inputFilename, 
                                    const FString &outputFilename, 
                                    const StrStrMap *environment) : 
	mCommand(command), 
	mCurrentWorkingDirectory(currentWorkingDirectory),
	mInputFilename(inputFilename),
	mOutputFilename(outputFilename),
	mGUID(GUID::GenerateGUID()),
	mChildPid(-1),
	mOutputString(""),
	mIsRunning(false),
	mFinishedCond(mFinishedLock)
{
    // copy the environment entries
    if(environment) {
        mEnvironment.insert(environment->begin(), environment->end());
    }

}

Forte::ProcessHandle::~ProcessHandle() 
{
}
	
void Forte::ProcessHandle::SetProcessCompleteCallback(ProcessCompleteCallback processCompleteCallback)
{
    mProcessCompleteCallback = processCompleteCallback;
}

void Forte::ProcessHandle::SetCurrentWorkingDirectory(const FString &cwd) 
{
    mCurrentWorkingDirectory = cwd;
}

void Forte::ProcessHandle::SetEnvironment(const StrStrMap *env)
{
    if(env) {
        mEnvironment.clear();
        mEnvironment.insert(env->begin(), env->end());
    }
}

void Forte::ProcessHandle::SetInputFilename(const FString &infile)
{
    mInputFilename = infile;
}

void Forte::ProcessHandle::SetProcessManager(ProcessManager* pm)
{
	mProcessManager = pm;
}
	
pid_t Forte::ProcessHandle::Run() 
{
    hlog(HLOG_DEBUG, "Running child process");
	AutoUnlockMutex lock(mFinishedLock);
	sigset_t set;
	
	mChildPid = fork();
	if(mChildPid < 0) {
        hlog(HLOG_ERR, "unable to fork child process");
		throw EProcessHandleUnableToFork();
	} else if(mChildPid == 0) {
		// this is the child
		mIsRunning = true;
		char *argv[ARG_MAX];
		
		// we need to split the command up
		std::vector<std::string> strings;
		boost::split(strings, mCommand, boost::is_any_of("\t "));
		unsigned int num_args = strings.size();
		for(unsigned int i = 0; i < num_args; ++i) {
			// ugh - I hate this cast here, but as this process
			// is about to be blown away, it is harmless, and
			// much simpler than trying to avoid it
			argv[i] = const_cast<char*>(strings[i].c_str());
		}
		argv[num_args] = 0;
		
		// change current working directory
        if (!mCurrentWorkingDirectory.empty()) {
            if (chdir(mCurrentWorkingDirectory)) {
                hlog(HLOG_CRIT, "Cannot change directory to: %s", mCurrentWorkingDirectory.c_str());
                cerr << "Cannot change directory to: " << mCurrentWorkingDirectory << endl;
                exit(-1);
            }
        }
		
		
		
		// setup in, out, err
		int inputfd = -1;
		while ((inputfd = open(mInputFilename, O_RDWR)) < 0 && errno == EINTR);
        if (inputfd != -1) {
            while (dup2(inputfd, 0) == -1 && errno == EINTR);
		}

		int outputfd = -1;
		while ((outputfd = open(mOutputFilename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) < 0 && errno == EINTR);
        if (outputfd != -1) {
            while (dup2(outputfd, 1) == -1 && errno == EINTR);
            while (dup2(outputfd, 2) == -1 && errno == EINTR);
		}
		
		// close all other FDs
		int fdstart = 3;
		int fdlimit = sysconf(_SC_OPEN_MAX);
		while (fdstart < fdlimit) 
        {
			close(fdstart++);
        }
		
		// set up environment?
        if (!mEnvironment.empty()) {
            StrStrMap::const_iterator mi;
			
            for (mi = mEnvironment.begin(); mi != mEnvironment.end(); ++mi) {
                if (mi->second.empty()) unsetenv(mi->first);
                else setenv(mi->first, mi->second, 1);
            }
        }
		
        // clear sig mask
        sigemptyset(&set);
        pthread_sigmask(SIG_SETMASK, &set, NULL);
		
		// create a new process group / session
        setsid();
		
		execv(argv[0], argv);
		throw EProcessHandleExecvFailed();
	} else {
		// this is the parent
		// this just returns back to whence it was called
		// the ProcessManager will now carry out the task
		// of monitoring the running process.
		mIsRunning = true;
		mProcessManager->RunProcess(mGUID);
	}
	return mChildPid;

}


unsigned int Forte::ProcessHandle::Wait()
{
    if(!mIsRunning) {
        throw EProcessHandleProcessNotRunning();
    }

    hlog(HLOG_DEBUG, "waiting for process to end (%s)", mGUID.c_str());
	AutoUnlockMutex lock(mFinishedLock);
	if(!mIsRunning) 
    {
		return mStatusCode;
	}
	mFinishedCond.Wait();
	return mStatusCode;
}

void Forte::ProcessHandle::Cancel()
{
    hlog(HLOG_DEBUG, "canceling process %u (%s)", mChildPid, mGUID.c_str());
	kill(mChildPid, SIGINT);
	Wait();
}

void Forte::ProcessHandle::Abandon(bool signal)
{
    hlog(HLOG_DEBUG, "abandoning process %u (%s)", mChildPid, mGUID.c_str());
	if(signal) 
    {
		kill(mChildPid, SIGINT);
	}
	mProcessManager->AbandonProcess(mGUID);

}

bool Forte::ProcessHandle::IsRunning()
{
    return mIsRunning;
}

void Forte::ProcessHandle::SetIsRunning(bool running)
{
	if(mIsRunning && !running) {
		// we have gone from a running state to a non-running state
		// we must be finished! grab the output
        hlog(HLOG_DEBUG, "process has terminated %u (%s)", mChildPid, mGUID.c_str());		
		
        AutoUnlockMutex lock(mFinishedLock);
		mFinishedCond.Broadcast();
		
	}
	mIsRunning = running;
	
}

FString Forte::ProcessHandle::GetOutputString()
{
    // lazy loading of the output string
    // check to see if the output string is empty
    // if so, and the output file wasn't the bit bucket
    // load it up and return it. Otherwise, we just load the
    // string we have already loaded
    if (mOutputString.empty() && mOutputFilename != "/dev/null") 
    {
        // read log file
        FString stmp;
        ifstream in(mOutputFilename, ios::in | ios::binary);
        char buf[4096];
		
        mOutputString.clear();
		
        while (in.good())
            {
                in.read(buf, sizeof(buf));
                stmp.assign(buf, in.gcount());
                mOutputString += stmp;
            }
        
        // cleanup
        in.close();
    }

    return mOutputString;
}

FString Forte::ProcessHandle::shellEscape(const FString& arg) 
{
    return arg;
}
