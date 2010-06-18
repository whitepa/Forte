#ifndef __ServerMain_h
#define __ServerMain_h

#include "Context.h"
#include "LogManager.h"
#include "ServiceConfig.h"
#include "AutoMutex.h"
#include "Exception.h"
#include <set>

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, ForteServerMainException);

    class ServerMain : public Object
    {
    public:
        ServerMain(int argc, char * const argv[], 
                   const char *getoptstr, const char *defaultConfig, 
                   bool daemonize = true);
        virtual ~ServerMain();

        virtual void MainLoop();
        virtual void Usage();
        void WritePidFile();
        void PrepareSigmask();
    
        FString mHostname;
        FString mConfigFile;
        FString mLogFile;
        FString mPidFile;
        bool mDaemon;
        LogManager mLogManager;
        Context mContext;
        Mutex mCallbackMutex;
        sigset_t mSigmask;
    };
};
#endif
