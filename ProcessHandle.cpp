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
    mInputFD(-1),
    mOutputFD(-1),
	mGUID(GUID::GenerateGUID()),
	mChildPid(-1),
	mOutputString(""),
    mStarted(false),
	mIsRunning(false),
	mFinishedCond(mFinishedLock)
{
    // copy the environment entries
    if(environment) 
    {
        mEnvironment.insert(environment->begin(), environment->end());
    }

}

Forte::ProcessHandle::~ProcessHandle() 
{

    // if something went wrong when we tried to run this process
    // there is a chance the input and output file descriptors are still open
    // let's try closing them here as a backup safety measure
    if(mInputFD != -1) 
    {
        hlog(HLOG_ERR, "the child input file descriptor is still set");
        int closeResult = -1;
        do
        {
            closeResult = close(mInputFD);
            if(closeResult == -1 && errno != EINTR)
            {
                hlog(HLOG_ERR, "unable to close input file (%d)", errno);
            }
        }
        while(closeResult == -1 && errno == EINTR);
    }
    
    if(mOutputFD != -1)
    {
        hlog(HLOG_ERR, "the child output file descriptor is still set");
        int closeResult = -1;
        do
        {
            closeResult = close(mOutputFD);
            if(closeResult == -1 && errno != EINTR)
            {
                hlog(HLOG_ERR, "unable to close output file (%d)", errno);
            }
        }
        while(closeResult == -1 && errno == EINTR);
    }

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
    if(env) 
    {
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

    // open the input and output files
    // these will throw if they are unable to open the files
    // and keep looping if they are interupped during the open
    do
    {
        mInputFD = open(mInputFilename, O_RDWR);
        if(mInputFD == -1 && errno != EINTR)
        {
            hlog(HLOG_ERR, "unable to open input file (%d)", errno);
            throw EProcessHandleUnableToOpenInputFile();
        }

    } 
    while (mInputFD == -1 && errno == EINTR);

    do
    {
        mOutputFD = open(mOutputFilename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        if(mOutputFD == -1 && errno != EINTR)
        {
            hlog(HLOG_ERR, "unable to open output file (%d)", errno);
            throw EProcessHandleUnableToOpenOutputFile();
        }

    } 
    while (mOutputFD == -1 && errno == EINTR);

	mChildPid = fork();
	if(mChildPid < 0) 
    {
        hlog(HLOG_ERR, "unable to fork child process");
		throw EProcessHandleUnableToFork();
	} 
    else if(mChildPid == 0) 
    {
		// this is the child
		mIsRunning = true;
		char *argv[ARG_MAX];
		
		// we need to split the command up
		std::vector<std::string> strings;
        FString scrubbedCommand = shellEscape(mCommand);
		boost::split(strings, scrubbedCommand, boost::is_any_of("\t "));
		unsigned int num_args = strings.size();
		for(unsigned int i = 0; i < num_args; ++i) 
        {
			// ugh - I hate this cast here, but as this process
			// is about to be blown away, it is harmless, and
			// much simpler than trying to avoid it
			argv[i] = const_cast<char*>(strings[i].c_str());
		}
		argv[num_args] = 0;
		
		// change current working directory
        if (!mCurrentWorkingDirectory.empty()) 
        {
            if (chdir(mCurrentWorkingDirectory)) 
            {
                hlog(HLOG_CRIT, "Cannot change directory to: %s", mCurrentWorkingDirectory.c_str());
                cerr << "Cannot change directory to: " << mCurrentWorkingDirectory << endl;
                exit(-1);
            }
        }
		
		// setup in, out, err
        int dupResult = -1;
        do
        {
            dupResult = dup2(mInputFD, 0);
            if(dupResult == -1 && errno != EINTR)
            {
                hlog(HLOG_ERR, "unable to duplicate the input file descriptor (%d)", errno);
                throw EProcessHandleUnableToDuplicateInputFD();
            }
        }
        while (dupResult == -1 && errno == EINTR);

        dupResult = -1;
        do
        {
            dupResult = dup2(mOutputFD, 1);
            if(dupResult == -1 && errno != EINTR)
            {
                hlog(HLOG_ERR, "unable to duplicate the output file descriptor (%d)", errno);
                throw EProcessHandleUnableToDuplicateOutputFD();
            }
        }
        while (dupResult == -1 && errno == EINTR);

        dupResult = -1;
        do
        {
            dupResult = dup2(mOutputFD, 2);
            if(dupResult == -1 && errno != EINTR)
            {
                hlog(HLOG_ERR, "unable to duplicate the error file descriptor (%d)", errno);
                throw EProcessHandleUnableToDuplicateErrorFD();
            }
        }
        while (dupResult == -1 && errno == EINTR);

		// close all other FDs
		int fdstart = 3;
		int fdlimit = sysconf(_SC_OPEN_MAX);
		while (fdstart < fdlimit) 
        {
            int closeResult = -1;
            do
            {
                closeResult = close(fdstart);
                if(closeResult == -1 && errno == EIO)
                {
                    hlog(HLOG_WARN, "unable to close parent file descriptor %d (EIO)", fdstart);
                }
            }
            while(closeResult == -1 && errno == EINTR);

            fdstart++;
            
        }
		
		// set up environment?
        if (!mEnvironment.empty()) {
            StrStrMap::const_iterator mi;
			
            for (mi = mEnvironment.begin(); mi != mEnvironment.end(); ++mi) 
            {
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
        hlog(HLOG_ERR, "unable to execv the command");
		throw EProcessHandleExecvFailed();
	} 
    else 
    {
		// this is the parent
        // close the input and output fds we opened for our child
        int closeResult = -1;
        do
        {
            closeResult = close(mInputFD);
            if(closeResult == -1 && errno != EINTR)
            {
                hlog(HLOG_ERR, "unable to close the child input file (%d)", errno);
                throw EProcessHandleUnableToCloseInputFile();
            }
            else 
            {
                mInputFD = -1;
            }
        }
        while(closeResult == -1 && errno == EINTR);

        closeResult = -1;
        do
        {
            closeResult = close(mOutputFD);
            if(closeResult == -1 && errno != EINTR)
            {
                hlog(HLOG_ERR, "unable to close the child output file (%d)", errno);
                throw EProcessHandleUnableToCloseOutputFile();
            }
            else
            {
                mOutputFD = -1;
            }
        }
        while(closeResult == -1 && errno == EINTR);

		// this just returns back to whence it was called
		// the ProcessManager will now carry out the task
		// of monitoring the running process.
        mStarted = true;
		mIsRunning = true;
		mProcessManager->RunProcess(mGUID);
	}
	return mChildPid;

}


unsigned int Forte::ProcessHandle::Wait()
{
    if(!mIsRunning) 
    {
        hlog(HLOG_ERR, "tried waiting on a process that is not current running");
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
    if(!mIsRunning)
    {
        hlog(HLOG_ERR, "tried canceling a process that is not current running");
        throw EProcessHandleProcessNotRunning();
    }
    hlog(HLOG_DEBUG, "canceling process %u (%s)", mChildPid, mGUID.c_str());
	kill(mChildPid, SIGINT);
	Wait();
}

void Forte::ProcessHandle::Abandon(bool signal)
{
    if(!mIsRunning)
    {
        hlog(HLOG_ERR, "tried abandoning a process that is not current running");
        throw EProcessHandleProcessNotRunning();
    }

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
    bool prevState = mIsRunning;
	mIsRunning = running;
	if(prevState && !mIsRunning) 
    {
		// we have gone from a running state to a non-running state
		// we must be finished! grab the output
        hlog(HLOG_DEBUG, "process has terminated %u (%s)", mChildPid, mGUID.c_str());		
		
        AutoUnlockMutex lock(mFinishedLock);
		mFinishedCond.Broadcast();
	}

	
}

unsigned int Forte::ProcessHandle::GetStatusCode() 
{ 
    if(!mStarted) {
        hlog(HLOG_ERR, "tried grabbing the status code from a process that hasn't been started");
        throw EProcessHandleProcessNotRunning();
    } else if(mIsRunning) {
        hlog(HLOG_ERR, "tried grabbing the status code from a process that hasn't completed yet");
        throw EProcessHandleProcessNotFinished();
    }
    return mStatusCode; 
}

ProcessTerminationType Forte::ProcessHandle::GetProcessTerminationType() 
{ 
    if(!mStarted || mIsRunning) {
        hlog(HLOG_ERR, "tried grabbing the termination type from a process that hasn't completed yet");
        throw EProcessHandleProcessNotFinished();
    }
    return mProcessTerminationType; 
}

FString Forte::ProcessHandle::GetOutputString()
{
    if(!mStarted || mIsRunning) {
        hlog(HLOG_ERR, "tried grabbing the output from a process that hasn't completed yet");
        throw EProcessHandleProcessNotFinished();
    }
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
