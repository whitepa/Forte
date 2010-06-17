#ifndef __ServerMain_h
#define __ServerMain_h

#include "Context.h"
#include "LogManager.h"
#include "ServiceConfig.h"
#include "AutoMutex.h"
#include "Callback.h"
#include "Exception.h"
#include <set>

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, EForteServerMain);

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
                   bool daemonize = true);


        virtual ~ServerMain();

        virtual void MainLoop();
        virtual void Usage();
        void WritePidFile();
        void PrepareSigmask();
    
        void RegisterShutdownCallback(Callback *callback);

        FString mHostname;
        FString mConfigFile;
        FString mLogFile;
        FString mPidFile;
        bool mDaemon;
        LogManager mLogManager;
        Context mContext;
        ServiceConfig mServiceConfig;
        Mutex mCallbackMutex;
        std::set<Callback*> mShutdownCallbacks;
        sigset_t mSigmask;

    private:
        /**
         * Private init called from constructor to init the object. called by both construct
         **/
        void init(const FString& configPath, int logMask);
    };
};
#endif
