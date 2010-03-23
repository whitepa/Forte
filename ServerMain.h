#ifndef __ServerMain_h
#define __ServerMain_h

#include "LogManager.h"
#include "ServiceConfig.h"
#include "AutoMutex.h"
#include "Callback.h"
#include "Exception.h"
#include <set>

// TODO: remove this:
volatile extern bool g_shutdown;

namespace Forte
{
    EXCEPTION_SUBCLASS(ForteException, ForteServerMainException);

    class ServerMain : public Object
    {
    public:
        ServerMain(int argc, char * const argv[], const char *getoptstr, const char *defaultConfig, bool daemonize = true);
        virtual ~ServerMain();

        virtual void MainLoop();
        virtual void Usage();
        void WritePidFile();
        void PrepareSigmask();
    
        void RegisterShutdownCallback(Callback *callback);

        static ServerMain &GetServer();
        static ServerMain *GetServerPtr();

        FString mHostname;
        FString mConfigFile;
        FString mLogFile;
        FString mPidFile;
        bool mDaemon;
        LogManager mLogManager;
        ServiceConfig mServiceConfig;
        Mutex mCallbackMutex;
        std::set<Callback*> mShutdownCallbacks;
        sigset_t mSigmask;
    protected:
        static Mutex sSingletonMutex;
        static ServerMain* sSingletonPtr;
    };
};
#endif
