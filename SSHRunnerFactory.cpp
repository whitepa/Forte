/* Copyright (c) 2008-2013 Scale Computing, Inc.  All Rights Reserved.
 *    Proprietary and Confidential */

#include "SSHRunnerFactory.h"
using namespace Forte;

SSHRunnerPtr SSHRunnerFactory::Create(const FString &SSHUsername,
                                      const FString &SSHPassword,
                                      const FString &IP,
                                      int SSHPort,
                                      unsigned int timeout)
{
    return Create(SSHUsername, SSHPassword, "", "", IP, SSHPort, timeout);
}

SSHRunnerPtr SSHRunnerFactory::Create(const FString &SSHUsername,
                                      const FString &SSHPassword,
                                      const FString &SSHPublicKeyFile,
                                      const FString &SSHPrivateKeyFile,
                                      const FString &IP,
                                      int SSHPort,
                                      unsigned int timeout)
{
    FTRACE;

    hlog(HLOG_INFO, "Waiting on Host %s for %d seconds", IP.c_str(), timeout);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(timeout);

    while (true)
    {
        try
        {
            SSHRunnerPtr sshRunner;
            if (SSHPublicKeyFile != "")
            {
                sshRunner=SSHRunnerPtr(new SSHRunner(SSHUsername,
                                                     SSHPublicKeyFile,
                                                     SSHPrivateKeyFile,
                                                     SSHPassword,
                                                     IP,
                                                     SSHPort));
            }
            else
            {
                sshRunner=SSHRunnerPtr(new SSHRunner(SSHUsername,
                                                     SSHPassword,
                                                     IP,
                                                     SSHPort));
            }

            hlog(HLOG_DEBUG2, "Host %s is up", IP.c_str());
            return sshRunner;
        }
        catch (Forte::Exception &e)
        {
            if (deadline.Expired())
            {
                hlog(HLOG_ERR, "Timed out waiting for host %s to be up (%s)",
                     IP.c_str(), e.what());
                throw;
            }
            if (hlog_ratelimit(10))
                hlog(HLOG_DEBUG, "Not yet up, caught %s on %s",
                     e.what(), IP.c_str());
            usleep(1000000);
        }
    }
}
