#include <csignal>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "ServerMain.h"
#include "FTrace.h"
#include "VersionManager.h"
#include "FileSystemImpl.h"
#include "SystemCallUtil.h"

using namespace std;
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
    // setup the config
    CNEW("forte.ServiceConfig", ServiceConfig);
    CGETNEWPTR("forte.FileSystem", FileSystemImpl, fs);

    mLogManager.SetGlobalLogMask(logMask);

    initHostname();

    // daemonize
    if (mDaemon && daemon(1,0)) {
        fprintf(stderr, "can't start as daemon: %s\n",
                SystemCallUtil::GetErrorDescription(errno).c_str());
        exit(1);
    }

    // read config file
    CGET("forte.ServiceConfig", ServiceConfig, sc);
    if (!mConfigFile.empty())
        sc.ReadConfigFile(mConfigFile);

    // pid file
    FString pidPath = sc.Get("pidfile");
    if (!pidPath.empty())
    {
        mPidFile = boost::make_shared<PidFile>(fs, pidPath, mDaemonName);
        mPidFile->Create();
    }

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
            fprintf(stderr, "Unable to get hostname: %s\n",
                    SystemCallUtil::GetErrorDescription(errno).c_str());
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
    FString logMask = sc.Get("logfile.level");
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
        if (!logMask.empty())
            mLogManager.SetLogMask(mLogFile, mLogManager.ComputeLogMaskFromString(logMask));
    }
    else
    {
        if (mDaemon)
        {
            mLogManager.BeginLogging();
        }
    }

    // set up specialized log levels for specified source code files
    FStringVector filenames;
    FStringVector levels;
    try
    {
        sc.GetVectorSubKey("logfile.special_levels", "filenames", filenames);
        sc.GetVectorSubKey("logfile.special_levels", "level", levels);
    }
    catch (EServiceConfigNoKey& e)
    {
        hlog(HLOG_DEBUG, "No special levels in config file: %s", e.what());
    }
    catch (...)
    {
        hlog(HLOG_ERR, "Unknown error occurred while looking up special log levels");
        throw;
    }

    if (filenames.size() != levels.size())
        throw EServerMainLogFileConfiguration("source level configuration invalid");
    for (unsigned int i = 0; i < filenames.size(); ++i)
    {
        FStringVector files;
        filenames[i].Explode(",", files, true);
        foreach (const FString &file, files)
        {
            mLogManager.SetSourceFileLogMask(file, mLogManager.ComputeLogMaskFromString(levels[i]));
        }
    }
    VersionManager::LogVersions();
}

ServerMain::~ServerMain()
{
    // delete the pidfile
    mContext.Remove("forte.ServiceConfig");
    mContext.Remove("forte.FileSystem");
}

void ServerMain::PrepareSigmask()
{
    FTRACE;
    sigemptyset(&mSigmask);
    sigaddset(&mSigmask, SIGINT);
    sigaddset(&mSigmask, SIGTERM);
    sigaddset(&mSigmask, SIGHUP);
    sigaddset(&mSigmask, SIGQUIT);
    sigaddset(&mSigmask, SIGALRM);

    if (pthread_sigmask(SIG_BLOCK, &mSigmask, NULL) == -1)
    {
        hlog(HLOG_ERR, "Could not set sigmask in ServerMain");
    }
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
                hlog(HLOG_DEBUG, "reopening log files");
                mLogManager.Reopen();
                hlog(HLOG_DEBUG, "log file reopened");
                break;
            default:
                hlog(HLOG_ERR,"Unhandled signal %d received.", sig);
            }
        }
        else if (errno == EINTR)
        {
            continue;
        }
        else if (errno != EAGAIN) // EAGAIN when timeout occurs
        {
            // should only be EINVAL (invalid timeout)
            hlog(HLOG_ERR, "Error while calling sigtimedwait (%s)",
                 SystemCallUtil::GetErrorDescription(errno).c_str());
        }
    }
}
