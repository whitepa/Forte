/* Copyright (c) 2008-2013 Scale Computing, Inc.  All Rights Reserved.
 *    Proprietary and Confidential */

#include "SSHRunnerFactory.h"
using namespace Forte;

SSHRunnerPtr SSHRunnerFactory::Create(const FString &sshUsername,
                                      const FString &sshPassword,
                                      const FString &IP,
                                      int sshPort,
                                      unsigned int timeout)
{
    return Create(sshUsername, sshPassword, "", "", IP, sshPort, timeout);
}

SSHRunnerPtr SSHRunnerFactory::Create(const FString &sshUsername,
                                      const FString &sshPassword,
                                      const FString &sshPublicKeyFile,
                                      const FString &sshPrivateKeyFile,
                                      const FString &IP,
                                      int sshPort,
                                      unsigned int timeout)
{
    FTRACE;

    hlog(HLOG_DEBUG2, "Waiting on Host %s for %d seconds", IP.c_str(), timeout);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(timeout);

    while (true)
    {
        try
        {
            SSHRunnerPtr sshRunner;
            if (sshPublicKeyFile != "")
            {
                sshRunner=SSHRunnerPtr(new SSHRunner(sshUsername,
                                                     sshPublicKeyFile,
                                                     sshPrivateKeyFile,
                                                     sshPassword,
                                                     IP,
                                                     sshPort));
            }
            else
            {
                sshRunner=SSHRunnerPtr(new SSHRunner(sshUsername,
                                                     sshPassword,
                                                     IP,
                                                     sshPort));
            }
            int remainingTime = deadline.GetRemainingSeconds();
            if (timeout)
            {
                if (remainingTime < 1)
                    sshRunner->SetTimeout(1);
                else
                    sshRunner->SetTimeout(remainingTime);
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
