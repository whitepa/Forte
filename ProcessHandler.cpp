// ProcessHandler.cpp

#include "ProcessHandler.h"
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

Forte::ProcessHandler::ProcessHandler(const FString &command, 
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
	mIsRunning(false),
	mFinishedCond(mFinishedLock)
{
    // copy the environment entries
    if(environment) {
        mEnvironment.insert(environment->begin(), environment->end());
    }

}

Forte::ProcessHandler::~ProcessHandler() 
{
}
	
void Forte::ProcessHandler::SetProcessCompleteCallback(ProcessCompleteCallback processCompleteCallback)
{
    mProcessCompleteCallback = processCompleteCallback;
}

void Forte::ProcessHandler::SetCurrentWorkingDirectory(const FString &cwd) 
{
    mCurrentWorkingDirectory = cwd;
}

void Forte::ProcessHandler::SetEnvironment(const StrStrMap *env)
{
    if(env) {
        mEnvironment.clear();
        mEnvironment.insert(env->begin(), env->end());
    }
}

void Forte::ProcessHandler::SetInputFilename(const FString &infile)
{
    mInputFilename = infile;
}

void Forte::ProcessHandler::SetProcessManager(ProcessManager* pm)
{
	mProcessManager = pm;
}
	
pid_t Forte::ProcessHandler::Run() 
{
	AutoUnlockMutex lock(mFinishedLock);
	sigset_t set;
	
	mChildPid = fork();
	if(mChildPid < 0) {
		throw EProcessHandlerUnableToFork();
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
			close(fdstart++);
		
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
		throw EProcessHandlerExecvFailed();
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


unsigned int Forte::ProcessHandler::Wait()
{
	AutoUnlockMutex lock(mFinishedLock);
	if(!mIsRunning) {
		return mStatusCode;
	}
	mFinishedCond.Wait();
	return mStatusCode;
}

void Forte::ProcessHandler::Cancel()
{
	kill(mChildPid, SIGINT);
	Wait();
}

void Forte::ProcessHandler::Abandon(bool signal)
{
	if(signal) {
		kill(mChildPid, SIGINT);
	}
	mProcessManager->AbandonProcess(mGUID);

}

bool Forte::ProcessHandler::IsRunning()
{
    return mIsRunning;
}

void Forte::ProcessHandler::SetIsRunning(bool running)
{
	AutoUnlockMutex lock(mFinishedLock);
	mIsRunning = running;
	if(!mIsRunning) {
		mFinishedCond.Broadcast();
	}
	
}

FString Forte::ProcessHandler::GetOutputString()
{
    return "[unknown]";
}

FString Forte::ProcessHandler::shellEscape(const FString& arg) 
{
    return arg;
}


/*
    // open temp log file
    if (log_child || (output != NULL))
    {
        stmp.Format("%u.%u.%u", (unsigned)getpid(), (unsigned)time(NULL), (unsigned)rand());
        filename.Format("/tmp/child-%s.log", stmp.c_str());
        unlink(filename);
    }


    if (output != NULL)
    {
        // read log file
        ifstream in(filename, ios::in | ios::binary);
        char buf[4096];

        output->clear();

        while (in.good())
        {
            in.read(buf, sizeof(buf));
            stmp.assign(buf, in.gcount());
            (*output) += stmp;
        }

        // cleanup
        in.close();
    }

    // log child output
    if (log_child)
    {
        // read log file
        ifstream in(filename);
        char buf[4096];
        size_t len, i = 0, n = 1000;

        while (in.good() && (i++ < n))
        {
            in.getline(buf, sizeof(buf));
            len = in.gcount();
            buf[sizeof(buf) - 1] = 0;
            if (len < sizeof(buf)) buf[len] = 0;
            if (log_child) hlog(HLOG_DEBUG, "CHILD: %s", buf);
        }

        if (i >= n) 
            hlog(HLOG_DEBUG, 
                 "Child output logging truncated at %llu lines", (u64) n);

        // cleanup
        in.close();
    }

    // delete that file
    if (log_child || output != NULL) unlink(filename);

    // done
    return ret;
}
*/

