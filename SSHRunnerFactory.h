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

            SSHRunnerPtr Create(const FString &sshUsername,
                                const FString &sshPassword,
                                const FString &IP,
                                int sshPort,
                                unsigned int timeout,
                                unsigned int retryDelay = 1);

            SSHRunnerPtr Create(const FString &sshUsername,
                                const FString &sshPassword,
                                const FString &sshPublicKeyFile,
                                const FString &sshPrivateKeyFile,
                                const FString &IP,
                                int sshPort,
                                unsigned int timeout,
                                unsigned int retryDelay = 1);
    };
};

#endif
