#include <csignal>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "ServerMain.h"
#include "FTrace.h"
#include "VersionManager.h"
using namespace Forte;
using namespace boost;

ServerMain::ServerMain(int argc, char * const argv[], 
                       const char *getoptstr, const char *defaultConfig,
                       bool daemonize) :
    mShutdown(false),
    mConfigFile(defaultConfig),
    mDaemonName(argv[0]),
    mDaemon(daemonize),
    mLogManager()
{
    // check command line
    int i = 0;
    optind = 0;
    int logMask = HLOG_NODEBUG;
    while (!i)
    {
        int c;
        c = getopt(argc, argv, getoptstr);
        switch(c)
        {
        case -1:
            i = 1;
            break;
        case 'c':
            mConfigFile.assign(optarg);
            break;
        case 'd':
            mDaemon = false;
            break;
        case 'v':
            logMask = HLOG_ALL;
            break;
            /*default:
              if (!strchr(getoptstr, c))
              {
              Usage();
              exit(1);
              break;
              }*/
        }
    }

    init(defaultConfig, logMask);
}

ServerMain::ServerMain(const FString& defaultConfig,
                       int logMask,
                       const FString& daemonName,
                       bool daemonize)
    : mShutdown(false), 
      mConfigFile(defaultConfig),
      mDaemonName(daemonName),
      mDaemon(daemonize),
      mLogManager()
{
    init(defaultConfig, logMask);
}

void ServerMain::init(const FString& defaultConfig,
                      int logMask)
{
    mLogManager.InitGlobal();

    // setup the config
    CNEW("forte.ServiceConfig", ServiceConfig);

    mLogManager.SetGlobalLogMask(logMask);

    initHostname();

    // daemonize
    if (mDaemon && daemon(1,0)) {
        fprintf(stderr, "can't start as daemon: %s\n", strerror(errno));
        exit(1);
    }

    // read config file
    CGET("forte.ServiceConfig", ServiceConfig, sc);
    if (!mConfigFile.empty())
        sc.ReadConfigFile(mConfigFile);

    // pid file
    mPidFile = sc.Get("pidfile");
    if (!mPidFile.empty()) WritePidFile();

    initLogging();
    hlog(HLOG_INFO, "%s is starting up", mDaemonName.c_str());

    // setup sigmask
    PrepareSigmask();
}

void ServerMain::initHostname()
{
    // get the hostname
    {
        char hn[128];    
        if (gethostname(hn, sizeof(hn)) < 0)
        {
            fprintf(stderr, "Unable to get hostname: %s\n", strerror(errno));
            exit(1);
        }
        mHostname.assign(hn);
        mHostname = mHostname.Left(mHostname.find_first_of("."));
        hlog(HLOG_INFO, "determined hostname to be '%s'", mHostname.c_str());
    }
}

void ServerMain::initLogging()
{
    CGET("forte.ServiceConfig", ServiceConfig, sc);

    // setup logging
    // set logfile path
    //mLogFile = sc.Get("logfile.path");
    mLogFile = sc.Get("logfile.path");

    // if we are not running as a daemon, use the log level
    // the conf file
    FString stmp, logMask;
    if ((stmp = sc.Get("logfile.level")) != "")
    {
        //TODO: move this logic into the log manager
        if (stmp.MakeUpper() == "ALL")
        {
            mLogManager.SetGlobalLogMask(HLOG_ALL);
        }
        else if (stmp.MakeUpper() == "NODEBUG")
        {
            mLogManager.SetGlobalLogMask(HLOG_NODEBUG);
        }
        else if (stmp.MakeUpper() == "MOST")
        {
            mLogManager.SetGlobalLogMask(HLOG_ALL ^ HLOG_DEBUG4);
            hlog(HLOG_INFO, "most (all xor DEBUG4)");
        }
        else
        {
            mLogManager.SetGlobalLogMask(strtoul(stmp, NULL, 0));
        }
    }

    if (!mDaemon)
    {
        // log to stderr if running interactively, use logmask as set
        // by CServerMain
        mLogManager.BeginLogging("//stderr");
    }

    //if (!mLogFile.empty())
    if (mLogFile.length() > 0)
    {
        mLogManager.BeginLogging(mLogFile);
    }
    else
    {
        if (mDaemon)
        {
            mLogManager.BeginLogging();
        }
    }

    VersionManager::LogVersions();
}

ServerMain::~ServerMain()
{
    // delete the pidfile
    unlink(mPidFile.c_str());
    mContext.Remove("forte.ServiceConfig");
}

void ServerMain::Usage()
{
    cout << "Incorrect usage." << endl;
}
void ServerMain::WritePidFile()
{
    // \TODO:  get rid of this method.  See Forte::PidFile
    FILE *file;
    if ((file = fopen(mPidFile.c_str(), "r")) != NULL)
    {
        int old_pid;
        if (fscanf(file, "%u", &old_pid) == 1)
        {
            char procfile[32];
            fclose(file);
            
            // make sure the pid is different
            // (if it's the same as our pid, 
            // this is obviously a stale pid file)
            if (getpid() != old_pid)
            {
                sprintf(procfile, "/proc/%u/cmdline", old_pid);
                if ((file = fopen(procfile, "r"))
                    != NULL)
                {
                    char cmdbuf[100];
                    fgets(cmdbuf, sizeof(cmdbuf), file);
                    fclose(file);
                    // XXX fix this
                    if (strstr(cmdbuf, "forte_process"))
                    {
                        // process is running
                        FString err;
                        err.Format("forte process already running on pid %u\n",
                                   old_pid);
                        throw EForteServerMain(err);
                    }
                }
            }
        }
        else
            fclose(file);
    }
    // create PID file
    if ((file = fopen(mPidFile.c_str(), "w")) == NULL)
    {
        FString err;
        err.Format("Unable to write to pid file %s\n", mPidFile.c_str());
        throw EForteServerMain(err);
    }
    else
    {
        fprintf(file, "%u", getpid());
        fclose(file);
    }
}

void ServerMain::PrepareSigmask()
{
    sigemptyset(&mSigmask);
    sigaddset(&mSigmask, SIGINT);
    sigaddset(&mSigmask, SIGTERM);
    sigaddset(&mSigmask, SIGHUP);
    sigaddset(&mSigmask, SIGQUIT);
    sigaddset(&mSigmask, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &mSigmask, NULL);
}

void ServerMain::Shutdown()
{
    FTRACE;
    
    mShutdown = true;
}

void ServerMain::MainLoop()
{
    FTRACE;

    // loop to receive signals
    int quit = 0;
    int sig;
    siginfo_t siginfo;
    struct timespec timeout;

    timeout.tv_sec=0;
    timeout.tv_nsec=100000000; // 100 ms

    hlog(HLOG_DEBUG, "quit=%i, mShutdown=%s", quit, 
         (mShutdown) ? "true" : "false");

    while (!quit && !mShutdown)
    {
//        sigwait(&mSigmask, &sig);
        if (sigtimedwait(&mSigmask, &siginfo, &timeout) >= 0)
        {
            sig = siginfo.si_signo;
            switch(sig)
            {
            case SIGINT:
            case SIGTERM:
            case SIGQUIT:
                hlog(HLOG_INFO,"Quitting due to signal %d.", sig);
                quit = 1;
                break;
            case SIGHUP:
                // log rotation has occurred XXX need to lock logging system
                hlog(HLOG_INFO, "reopening log files");
                mLogManager.Reopen();
                hlog(HLOG_INFO, "log file reopened");
                break;
            default:
                hlog(HLOG_ERR,"Unhandled signal %d received.", sig);
            }
        }
        else if (errno != EAGAIN) // EAGAIN when timeout occurs
        {
            // should only be EINTR (interrupted with a signal not in sig mask)
            //                EINVAL (invalid timeout)
            hlog(HLOG_ERR, "Error while calling sigtimedwait (%s)", 
                 strerror(errno));
     
        }
    } 
}
