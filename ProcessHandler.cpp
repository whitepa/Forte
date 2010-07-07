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
				    const FString &currentWorkingDirectory ,
				    const StrStrMap *environment,
				    const FString &inputFilename) :
  mCommand(command), 
  mCurrentWorkingDirectory(currentWorkingDirectory),
  mInputFilename(inputFilename),
  mGUID(GUID::GenerateGUID()),
  mChildPid(-1),
  mIsRunning(false)
{
    // copy the environment entries
    if(environment) {
        mEnvironment.insert(environment->begin(), environment->end());
    }

}

Forte::ProcessHandler::~ProcessHandler() 
{
	FTRACE;
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
    // need to set everything up, fork/exec, and then hook up the 
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
	while(mIsRunning) {
		sleep(1);
	}
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
	mIsRunning = running;
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
// methods
int ProcessManager::Run(const FString& command, 
                    const FString& cwd, 
                    FString *output, 
                    unsigned int timeout,
                    const StrStrMap *env,
                    const FString &infile)
{
    if (cwd.empty()) hlog(HLOG_DEBUG3, "run(%s)", command.c_str());
    else hlog(HLOG_DEBUG3, "run(%s) [in %s]", command.c_str(), cwd.c_str());
    LogTimer timer(HLOG_DEBUG3, "Child ran for @TIME@");
    vector<FString>::const_iterator vi;
    FString stmp, filename, path = "/bin/bash";
    int ret = -1, fd_out = 1;
    bool log_child = false;
    sigset_t set;
    pid_t pid;

    // \TODO re-enable this; or just re-write this whole class.
    // // get default settings
    // try
    // {
    //     ServerMain& main = ServerMain::GetServer();
    //     log_child = (main.mServiceConfig.GetInteger("log_child") != 0);
    // }
    // catch (Exception &e)
    // {
    //     // there won't always be a ServerMain defined
    // }

    // open temp log file
    if (log_child || (output != NULL))
    {
        stmp.Format("%u.%u.%u", (unsigned)getpid(), (unsigned)time(NULL), (unsigned)rand());
        filename.Format("/tmp/child-%s.log", stmp.c_str());
        unlink(filename);
    }

    // fork and run the given child process
    if ((pid = fork()) == 0)
    {
        // child

        // in the child proc, errors should end up calling exit()
        // DO NOT THROW EXCEPTIONS

        unsigned i = 0;
        char **vargs = new char* [4];

        // create argv[0]
        vargs[i] = new char [path.length() + 1];
        strncpy(vargs[i], path.c_str(), path.length());
        vargs[i][path.length()] = 0;
        i++;

        // create argv[1]
        vargs[i] = new char [3];
        strncpy(vargs[i], "-c", 3);
        vargs[i][2] = 0;
        i++;

        // create argv[2]
        vargs[i] = new char [command.length() + 1];
        strncpy(vargs[i], command.c_str(), command.length());
        vargs[i][command.length()] = 0;
        i++;

        // redirect stdout, stderr
        if (log_child || output != NULL)
        {
            fd_out = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
            if (fd_out != -1) dup2(fd_out, 1);
            if (fd_out != -1) dup2(fd_out, 2);
        }

        // redirect input from given file (/dev/null by default)
        int nullfd;
        while ((nullfd = open(infile, O_RDWR)) < 0 && errno == EINTR);
        if (nullfd != -1)
            while (dup2(nullfd, 0) == -1 && errno == EINTR);

        // close all other fds
        // TODO: pick the upper bound more intelligently: need to check ulimit
        //       for max open file descriptors for this process.
        for (int n = 3; n < 1024; ++n)
            while (close(n) == -1 && errno == EINTR);

        // change current working directory
        if (!cwd.empty())
        {
            if (chdir(cwd))
            {
                hlog(HLOG_CRIT, "Cannot change directory to: %s", cwd.c_str());
                cerr << "Cannot change directory to: " << cwd << endl;
                exit(-1);
            }
        }

        // set up environment?
        if (env != NULL)
        {
            StrStrMap::const_iterator mi;

            for (mi = env->begin(); mi != env->end(); ++mi)
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

        // execute program
        vargs[i] = NULL;
        execv(path.c_str(), vargs);

        // anything that gets here is a failure
        exit(-1);
    }

    // parent
    if (pid < 0)
    {
        stmp.Format("Cannot execute command: %s", command.c_str());
        hlog(HLOG_ERR, "%s", stmp.c_str());
        unlink(filename);
        return ret;
    }

    // wait for the child to complete, while checking its heartbeat
    int res, status, sig;
    unsigned sec, usec, total_msec;
    struct rusage usage;

    //OLD WAY
    //int elapsed;
    //struct timeval start_time, current_time;
    //time_t start = time(NULL);
    struct timespec sleep_time;

    struct timespec start_time, current_time, elapsed_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    //gettimeofday(&start_time, NULL);

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);

    while (true)
    {
        // get child's status
        if ((res = wait4(pid, &status, WNOHANG, &usage)) != 0)
        {
            ret = WEXITSTATUS(status);

            if (WIFSIGNALED(status))
            {
                ret = -1;
                stmp.Format("Child exited with signal %u", WTERMSIG(status));
                hlog(HLOG_ERR, "%s", stmp.c_str());
                break;
            }

            if (WIFEXITED(status))
            {
                if (WEXITSTATUS(status) == 255)
                {
                    stmp.Format("Child failed to run (exit code -1)");
                    hlog(HLOG_ERR, "%s", stmp.c_str());
                    break;
                }

                if (WEXITSTATUS(status) != 0)
                {
                    stmp.Format("Child exited with status code %d", 
                                WEXITSTATUS(status));
                    hlog(HLOG_DEBUG, "%s", stmp.c_str());
                    break;
                }

                hlog(HLOG_DEBUG, "Child exited normally");
                break;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &current_time);
        elapsed_time = current_time - start_time;

        // check for timeout
        if (timeout != 0)
        {
            //OLD WAY
            //elapsed = time(NULL) - start;
            //if (elapsed > timeout)
            if (elapsed_time.tv_sec > timeout)
            {
                if (killpg(pid, 15) == -1)
                {
                    if (killpg(pid, 9) == -1)
                    {
                        hlog(HLOG_ERR, "failed to killpg(%u): %s",
                             (unsigned)pid, strerror(errno));
                        hlog(HLOG_DEBUG, "calling kill(%u, 9)", (unsigned)pid);

                        if (kill(pid, 9) == -1)
                        {
                            hlog(HLOG_ERR, "failed to kill -9 (%u): %s",
                                 (unsigned)pid, strerror(errno));
                        }
                    }
                }
                wait4(pid, &status, 0, &usage);  // wait for death to
                // clean up zombie
                sec = usage.ru_utime.tv_sec + usage.ru_stime.tv_sec;
                usec = usage.ru_utime.tv_usec + usage.ru_stime.tv_usec;
                if (usec > 1000000) { sec++; usec -= 1000000; }
                stmp.Format("Child process has timed out\n"
                            "Total CPU Time: %u.%03u seconds\n"
                            "Timeout is set to %d seconds",
                            sec, usec / 1000,
                            timeout);
                hlog(HLOG_ERR, "%s", stmp.c_str());
                ret = -1;
                break;
            }
        }

        // check for shutdown
        // TODO:  g_shutdown has been removed!

        // determine how long to sleep before checking process for
        // termination again

        //OLD WAY
        //gettimeofday(&current_time, NULL);
        //total_msec = 1000 * (current_time.tv_sec - start_time.tv_sec);
        //total_msec += (current_time.tv_usec - start_time.tv_usec + 500) / 1000;
        total_msec = 
            (elapsed_time.tv_nsec + (elapsed_time.tv_sec * 1000000000))
            / 1000000;

        if (total_msec < 1000)
        {
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = 10000000;   // 10 milliseconds
        }
        else if (total_msec < 2000)
        {
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = 100000000;  // 100 milliseconds
        }
        else if (total_msec < 5000)
        {
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = 250000000;  // 250 milliseconds
        }
        else if (total_msec < 10000)
        {
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = 500000000;  // 500 milliseconds
        }
        else
        {
            sleep_time.tv_sec = 1;           // 1 second
            sleep_time.tv_nsec = 0;
        }

        // wait for a while
        sig = sigtimedwait(&set, NULL, &sleep_time);
    }

    // save output?
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


int ProcessManager::RunBackground(const FString& command, const FString& cwd, const StrStrMap *env)
{
    if (cwd.empty()) hlog(HLOG_DEBUG3, "run_background(%s)", command.c_str());
    else hlog(HLOG_DEBUG3, "run_background(%s) [in %s]", command.c_str(), cwd.c_str());
    vector<FString>::const_iterator vi;
    FString stmp, path = "/bin/bash";
    int status, ret = -1;
    sigset_t set;
    pid_t pid;

    // fork and run the given child process
    if ((pid = fork()) == 0)
    {
        // child

        // fork into background
        daemon(1, 0);

        // vars
        unsigned i = 0;
        char **vargs = new char* [4];

        // create argv[0]
        vargs[i] = new char [path.length() + 1];
        strncpy(vargs[i], path.c_str(), path.length());
        vargs[i][path.length()] = 0;
        i++;

        // create argv[1]
        vargs[i] = new char [3];
        strncpy(vargs[i], "-c", 3);
        vargs[i][2] = 0;
        i++;

        // create argv[2]
        vargs[i] = new char [command.length() + 1];
        strncpy(vargs[i], command.c_str(), command.length());
        vargs[i][command.length()] = 0;
        i++;

        // change current working directory?
        if (!cwd.empty())
        {
            if (chdir(cwd))
            {
                hlog(HLOG_CRIT, "Cannot change directory to: %s", cwd.c_str());
                cerr << "Cannot change directory to: " << cwd << endl;
                exit(-1);
            }
        }

        // redirect in/out/err to/from /dev/null
        int nullfd;
        while ((nullfd = open("/dev/null", O_RDWR)) < 0 && errno == EINTR);
        if (nullfd != -1)
        {
            while (dup2(nullfd, 0) == -1 && errno == EINTR);
            while (dup2(nullfd, 1) == -1 && errno == EINTR);
            while (dup2(nullfd, 2) == -1 && errno == EINTR);
        }

        // close all fds
        // TODO: pick the upper bound more intelligently: need to check ulimit
        //       for max open file descriptors for this process.
        for (int n = 3; n < 1024; ++n)
            while (close(n) == -1 && errno == EINTR);

        // set up environment?
        if (env != NULL)
        {
            StrStrMap::const_iterator mi;

            for (mi = env->begin(); mi != env->end(); ++mi)
            {
                if (mi->second.empty()) unsetenv(mi->first);
                else setenv(mi->first, mi->second, 1);
            }
        }

        // clear sig mask
        sigemptyset(&set);
        pthread_sigmask(SIG_BLOCK, &set, NULL);

        // create new process group / session
        setsid();

        // execute program
        vargs[i] = NULL;
        execv(path.c_str(), vargs);

        // anything that gets here is a failure
        exit(-1);
    }

    // parent

    // check fork status
    if (pid < 0)
    {
        stmp.Format("Cannot execute command: %s", command.c_str());
        hlog(HLOG_ERR, "%s", stmp.c_str());
        return ret;
    }

    // wait for child to fork into background
    if (waitpid(pid, &status, 0) == -1)
    {
        stmp.Format("Unable to verify status of command: %s", command.c_str());
        hlog(HLOG_ERR, "%s", stmp.c_str());
        return ret;
    }

    // done
    return 0;
}


FString ProcessManager::ReadPipe(const FString& command, int *exitval)
{
    FILE *fp;
    FString ret;
    char buf[16384];
    std::vector<char> data;
    size_t r, l;
    int e;

    // open pipe
    if ((fp = popen(command, "r")) == NULL) return ret;

    // read data
    while (!feof(fp))
    {
        r = fread(buf, 1, sizeof(buf), fp);
        l = data.size();
        data.resize(l + r);
        memcpy((&data.front()) + l, buf, r);
    }

    // get exit value
    e = pclose(fp);
    if (exitval != NULL) *exitval = e;

    // set return value
    ret.assign(&data.front(), data.size());

    // done
    return ret;
}


FString ProcessManager::ShellEscape(const FString& arg)
{
    FString ret(arg);
    ret.Replace("'", "'\\''");
    return "'" + ret + "'";
}


int ProcessManager::Rsh(const FString& ip, const FString& command, FString *output, int timeout)
{
    FString cmd, esc, tmpout, stmp;
    size_t pos;
    int err;

    esc = ShellEscape(command + " ; echo $?");
    cmd.Format("rsh %s %s", ip.c_str(), esc.c_str());
    err = Run(cmd, "", &tmpout, timeout);
    tmpout.Trim();
    pos = tmpout.find_last_of('\n');
    if (output != NULL) *output = tmpout.Left((pos == NOPOS) ? 0 : pos);
    if (err != 0) return err;
    err = tmpout.Mid(pos + 1).AsInteger();
    return err;
}


int ProcessManager::Ssh(const FString& host, const FString& command, FString *output, int timeout)
{
    FString cmd, esc;
    esc = ShellEscape(command);
    cmd.Format("ssh %s %s", host.c_str(), esc.c_str());
    return Run(cmd, "", output, timeout);
}

*/
