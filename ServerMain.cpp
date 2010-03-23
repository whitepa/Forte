#include "Forte.h"
#include <csignal>

volatile bool g_shutdown = false;

Mutex       ServerMain::sSingletonMutex;
ServerMain* ServerMain::sSingletonPtr;

ServerMain::ServerMain(int argc, char * const argv[], 
                         const char *getoptstr, const char *defaultConfig,
                         bool daemonize) :
    mConfigFile(defaultConfig),
    mDaemon(daemonize)
{
    // set the singleton pointer
    {
        AutoUnlockMutex lock(sSingletonMutex);
        if (sSingletonPtr != NULL)
            throw ForteServerMainException("only one ServerMain object may exist");
        else
            sSingletonPtr = this;
    }
    mLogManager.SetGlobalLogMask(HLOG_NODEBUG); // suppress debug logging
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
    // check command line
    int i = 0;
    optind = 0;
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
            mLogManager.SetGlobalLogMask(HLOG_ALL); // verbose logging
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
    // daemonize
    if (mDaemon && daemon(1,0)) {
        fprintf(stderr, "can't start as daemon: %s\n", strerror(errno));
        exit(1);
    }
    // read config file
    if (!mConfigFile.empty())
        mServiceConfig.ReadConfigFile(mConfigFile);
    
}

ServerMain::~ServerMain()
{
    g_shutdown = true;

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
            (*i)->execute();
            // free the callback
            delete (*i);
        }
    }

    // delete the pidfile
    unlink(mPidFile.c_str());

    // unset the singleton pointer
    AutoUnlockMutex lock(sSingletonMutex);
    sSingletonPtr = NULL;
}

void ServerMain::RegisterShutdownCallback(Callback *callback)
{
    hlog(HLOG_DEBUG, "registering shutdown callback");
    AutoUnlockMutex lock(mCallbackMutex);
    mShutdownCallbacks.insert(callback);
}

ServerMain& ServerMain::GetServer()
{
    // return a reference to the ServerMain object
    AutoUnlockMutex lock(sSingletonMutex);
    if (sSingletonPtr == NULL)
        throw ForteEmptyReferenceException("ServerMain object does not exist");
    else
        return *sSingletonPtr;
}

ServerMain* ServerMain::GetServerPtr()
{
    // return a pointer to the ServerMain object
    AutoUnlockMutex lock(sSingletonMutex);
    if (sSingletonPtr == NULL)
        throw ForteEmptyReferenceException("ServerMain object does not exist");
    else
        return sSingletonPtr;
}

void ServerMain::Usage()
{
    cout << "Incorrect usage." << endl;
}
void ServerMain::WritePidFile()
{
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
                        throw ForteServerMainException(err);
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
        throw ForteServerMainException(err);
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

    g_shutdown = true;
}
