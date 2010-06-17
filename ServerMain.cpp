#include "Forte.h"
#include <csignal>
#include <boost/make_shared.hpp>

using namespace Forte;

ServerMain::ServerMain(int argc, char * const argv[], 
                       const char *getoptstr, const char *defaultConfig,
                       bool daemonize) :
    mConfigFile(defaultConfig),
    mDaemon(daemonize)
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
        default:
            if (!strchr(getoptstr, c))
            {
                Usage();
                exit(1);
                break;
            }
        }
    }

    init(defaultConfig, logMask);
}


ServerMain::ServerMain(const FString& defaultConfig,
                       int logMask,
                       bool daemonize)
     : mConfigFile(defaultConfig),
       mDaemon(daemonize)
{
    init(defaultConfig, logMask);
}

void ServerMain::init(const FString& defaultConfig,
                      int logMask)
{
    // set the singleton pointer
    // TODO: get rid of this singleton stuff.
    {
        AutoUnlockMutex lock(sSingletonMutex);
        if (sSingletonPtr != NULL)
            throw EForteServerMain("only one ServerMain object may exist");
        else
            sSingletonPtr = this;
    }

    mLogManager.SetGlobalLogMask(logMask);
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

    // daemonize
    if (mDaemon && daemon(1,0)) {
        fprintf(stderr, "can't start as daemon: %s\n", strerror(errno));
        exit(1);
    }
    // read config file
    if (!mConfigFile.empty())
<<<<<<< HEAD
        mServiceConfig.ReadConfigFile(mConfigFile);        
=======
        mServiceConfig.ReadConfigFile(mConfigFile);

    // pid file
    mPidFile = mServiceConfig.Get("pidfile");
    if (!mPidFile.empty()) WritePidFile();

    // setup logging
    // set logfile path
    mLogFile = mServiceConfig.Get("logfile.path");

    // if we are not running as a daemon, use the log level
    // the conf file
    FString stmp;
    if ((stmp = mServiceConfig.Get("logfile.level")) != "")
    {
        //TODO: move this logic into the log manager
        if (stmp.MakeUpper() == "ALL")
        {
            mLogManager.SetGlobalLogMask(HLOG_ALL);
            hlog(HLOG_INFO, "Log mask set to ALL");
        }
        else if (stmp.MakeUpper() == "NODEBUG")
        {
            mLogManager.SetGlobalLogMask(HLOG_NODEBUG);
            hlog(HLOG_INFO, "Log mask set to NODEBUG");
        }
        else
        {
            mLogManager.SetGlobalLogMask(strtoul(stmp, NULL, 0));
            hlog(HLOG_INFO, "Log mask set to 0x%08lx", strtoul(stmp, NULL, 0));
        }
    }
>>>>>>> overhaul_phase_2
}




ServerMain::~ServerMain()
{
    // call the shutdown callbacks
    hlog(HLOG_DEBUG, "ServerMain shutdown: calling shutdown callbacks...");
    {
        AutoUnlockMutex lock(mCallbackMutex);
        std::set<Callback*>::iterator i;
        for (i = mShutdownCallbacks.begin();
             i != mShutdownCallbacks.end();
             i++)
        {
            // execute the callback
            (*i)->Execute();
            // free the callback
            delete (*i);
        }
    }

    // delete the pidfile
    unlink(mPidFile.c_str());
}

void ServerMain::RegisterShutdownCallback(Callback *callback)
{
    hlog(HLOG_DEBUG, "registering shutdown callback");
    AutoUnlockMutex lock(mCallbackMutex);
    mShutdownCallbacks.insert(callback);
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

void ServerMain::MainLoop()
{
    // loop to receive signals
    int quit = 0;
    int sig;

    while (!quit)
    {
        sigwait(&mSigmask, &sig);

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
}
