#ifndef __ServerMain_h
#define __ServerMain_h

#include "ContextImpl.h"
#include "LogManager.h"
#include "ServiceConfig.h"
#include "Exception.h"
#include "PidFile.h"

#include <set>

namespace Forte
{
    EXCEPTION_CLASS(EServerMain);
    EXCEPTION_SUBCLASS2(EServerMain, EServerMainLogFileConfiguration,
                        "Logfile configuration is invalid");

    class ServerMain : public Object
    {
    public:
        ServerMain(int argc, char * const argv[],
                   const char *getoptstr, const char *defaultConfig,
                   bool daemonize = true);

        /**
         * Constructor to server main that takes neede parameters directly
         **/
        ServerMain(const FString& configPath,
                   int logMask = HLOG_NODEBUG,
                   const FString& daemonName = "",
                   bool daemonize = true);


        virtual ~ServerMain();

        virtual void MainLoop();
        virtual void PrepareSigmask();

        /**
         * Tell this server to shutdown
         */
        void Shutdown();
        bool mShutdown;

        FString mHostname;
        FString mConfigFile;
        FString mDaemonName;
        FString mLogFile;
        boost::shared_ptr<PidFile> mPidFile;
        bool mDaemon;
        LogManager mLogManager;
        ContextImpl mContext;
        sigset_t mSigmask;

    protected:
        void initHostname();
        void initLogging();

    private:
        /**
         * Private init called from constructor to init the object. called by both construct
         **/
        void init(const FString& configPath, int logMask);
    };
};
#endif
