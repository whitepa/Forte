#ifndef _SSHRunnerFactory_h
#define _SSHRunnerFactory_h

#include "FTrace.h"
#include "SSHRunner.h"

namespace Forte
{
    class SSHRunnerFactory
    {
        public:
            SSHRunnerFactory() { FTRACE; }
            ~SSHRunnerFactory() { FTRACE; }

            SSHRunnerPtr Create(const FString &SSHUsername,
                                const FString &SSHPassword,
                                const FString &IP,
                                int SSHPort,
                                unsigned int timeout);

            SSHRunnerPtr Create(const FString &SSHUsername,
                                const FString &SSHPassword,
                                const FString &SSHPublicKeyFile,
                                const FString &SSHPrivateKeyFile,
                                const FString &IP,
                                int SSHPort,
                                unsigned int timeout);
    };
};

#endif
