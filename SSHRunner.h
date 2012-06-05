/* Copyright (c) 2008-2010 Scale Computing, Inc.  All Rights Reserved.
   Proprietary and Confidential */

#ifndef _SSHRunner_h
#define _SSHRunner_h

#include "Exception.h"
#include "Object.h"
#include "FString.h"
#include "libssh2.h"

namespace Forte
{
    EXCEPTION_CLASS(ESSHRunner);
    EXCEPTION_SUBCLASS(ESSHRunner, ESocketError);
    EXCEPTION_SUBCLASS(ESSHRunner, ESessionError);
    EXCEPTION_SUBCLASS(ESSHRunner, ERunError);
    EXCEPTION_SUBCLASS(ESSHRunner, ESCPError);
    
    /**
     * Class to ssh into a box and run commands.
     * Has no dependencies on keys installed on the
     * box, and takes a username and password
     */
    class SSHRunner : public Object
    {
    public:
        /**
         * Constructor
         *
         * @param username
         * @param password
         * @param ipAddress
         * @param portNumber
         */
        SSHRunner(
            const char *username,
            const char *password,
            const char *ipAddress,
            int portNumber);

        /*
         * Constructor for public key authentication
         */
        SSHRunner(const char *username,
                  const char *publicKeyFilePath,
                  const char *privateKeyFilePath,
                  const char *passphrase,
                  const char *ipAddress,
                  int portNumber);

        /**
         * Destructor will destroy the ssh session
         */
        virtual ~SSHRunner();

        /**
         * This function runs a command on the remote box
         * using the ssh session created.
         *
         * @param command command to run
         * @param output the std output (OUT)
         * @param errorOutput the error output (OUT)
         * @return the exit code of the command
         */
        virtual int Run(const FString &command,
                        FString *output,
                        FString *errorOutput);

        virtual void GetFile(const FString &remotePath, 
                             const FString &loclPath);

        FString GetIPAddress() {
            return mIPAddress;
        }

    protected:
        /**
         * Empty constructor
         * Mostly for mock object which might inherit.
         */
        SSHRunner();

        LIBSSH2_SESSION *mSession;
        int mSocket;
        FString mIPAddress;
        int mPort;

        int createSocketAndConnect(const char *ipAddress, int portNumber); 
        int waitSocket(int socket_fd, LIBSSH2_SESSION *session);
        FString getErrorString();
    };

    typedef boost::shared_ptr<SSHRunner> SSHRunnerPtr;
};

#endif
