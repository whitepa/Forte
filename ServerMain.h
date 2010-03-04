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
    EXCEPTION_SUBCLASS(CForteException, CForteServerMainException);

    class CServerMain : public Object
    {
    public:
        CServerMain(int argc, char * const argv[], const char *getoptstr, const char *defaultConfig, bool daemonize = true);
        virtual ~CServerMain();

        virtual void MainLoop();
        virtual void Usage();
        void WritePidFile();
        void PrepareSigmask();
    
        void RegisterShutdownCallback(CCallback *callback);

        static CServerMain &GetServer();
        static CServerMain *GetServerPtr();

        FString mHostname;
        FString mConfigFile;
        FString mLogFile;
        FString mPidFile;
        bool mDaemon;
        CLogManager mLogManager;
        CServiceConfig mServiceConfig;
        CMutex mCallbackMutex;
        std::set<CCallback*> mShutdownCallbacks;
        sigset_t mSigmask;
    protected:
        static CMutex sSingletonMutex;
        static CServerMain* sSingletonPtr;
    };
};
#endif
