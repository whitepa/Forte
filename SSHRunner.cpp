/* Copyright (c) 2008-2010 Scale Computing, Inc.  All Rights Reserved.
   Proprietary and Confidential */

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "FTrace.h"
#include "SSHRunner.h"
#include "LogManager.h"
#include "AutoFD.h"
#include "SystemCallUtil.h"

using namespace Forte;

SSHRunner::SSHRunner(const char *username,
                     const char *password,
                     const char *ipAddress,
                     int portNumber) :
    mSession (NULL),
    mSocket (-1)
{
    FTRACE2("Starting ssh session for %s:%d", ipAddress, portNumber);
    mSocket = createSocketAndConnect(ipAddress, portNumber);
    if (mSocket > 0)
    {
        mSession = libssh2_session_init();
        if (!mSession)
            hlog_and_throw(HLOG_ERR, ESessionError("Could not init ssh"));
        if (libssh2_session_startup(mSession, mSocket) < 0)
        {
            FString stmp(FStringFC(),
                         "Could not startup the ssh session to %s:%d (err=%s)",
                         ipAddress, portNumber, getErrorString().c_str());

            libssh2_session_free(mSession);
            close(mSocket);
            hlog(HLOG_ERR, "%s", stmp.c_str());
            throw ESessionError(stmp);
        }

        if (libssh2_userauth_password(mSession, username, password) < 0)
        {
            FString stmp(FStringFC(),
                  "Could not get authenticated using %s/%s to %s:%d (err=%s)",
                   username, password,
                   ipAddress, portNumber, getErrorString().c_str());

            libssh2_session_disconnect(mSession, "Goodbye");
            libssh2_session_free(mSession);
            close(mSocket);
            hlog(HLOG_ERR, "%s", stmp.c_str());
            throw ESessionError(stmp);
        }
    }
    else
    {
        FString stmp(FStringFC(),
                     "No socket for creating ssh session to %s:%d ",
                     ipAddress, portNumber);
        hlog(HLOG_ERR, "%s", stmp.c_str());
        throw ESessionError(stmp);
    }
}

SSHRunner::SSHRunner(const char *username,
                     const char *publicKeyFilePath,
                     const char *privateKeyFilePath,
                     const char *passphrase,
                     const char *ipAddress,
                     int portNumber) :
    mSession (NULL),
    mSocket (-1),
    mPort(-1)
{
    FTRACE2("%s, %s, %s, %s, %s, %i", username, publicKeyFilePath,
            privateKeyFilePath, passphrase, ipAddress, portNumber);
    mSocket = createSocketAndConnect(ipAddress, portNumber);
    if (mSocket > 0)
    {
        mSession = libssh2_session_init();
        if (!mSession)
            hlog_and_throw(HLOG_ERR, ESessionError("Could not init ssh"));
        if (libssh2_session_startup(mSession, mSocket) < 0)
        {
            FString stmp(FStringFC(),
                        "Could not startup the ssh session to %s:%d (err=%s)",
                        ipAddress, portNumber, getErrorString().c_str());

            libssh2_session_free(mSession);
            close(mSocket);
            hlog(HLOG_ERR, "%s", stmp.c_str());
            throw ESessionError(stmp);
        }

        if (libssh2_userauth_publickey_fromfile_ex(mSession, username,
                                                   strlen(username),
                                                   publicKeyFilePath,
                                                   privateKeyFilePath,
                                                   passphrase) < 0)
        {
            FString stmp(FStringFC(),
                         "Could not get authenticated using (%s)(%s)(%s) "
                         "to %s:%d (err=%s)",
                         username, publicKeyFilePath, privateKeyFilePath,
                         ipAddress, portNumber, getErrorString().c_str());

            libssh2_session_disconnect(mSession, "Goodbye");
            libssh2_session_free(mSession);

            close(mSocket);

            hlog(HLOG_ERR, "%s", stmp.c_str());
            throw ESessionError(stmp);
        }
    }
    else
    {
        FString stmp(FStringFC(),
                     "No socket for creating ssh session to %s:%d ",
                     ipAddress, portNumber);
        hlog(HLOG_ERR, "%s", stmp.c_str());
        throw ESessionError(stmp);
    }
}

SSHRunner::SSHRunner() :
    mSession (NULL),
    mSocket (0)
{
}

SSHRunner::~SSHRunner()
{
    if (mSession != NULL)
    {
        libssh2_session_disconnect(mSession, "Goodbye");
        libssh2_session_free(mSession);
        close(mSocket);
    }
}

bool SSHRunner::RunQuiet( const FString &command)
{
    FString output, errOutput;
    if (SSHRunner::Run(command, &output, &errOutput) != 0)
        return false;
    return true;
}

int SSHRunner::Run( const FString &command)
{
    FString output, errOutput;
    int ret;
    ret = SSHRunner::Run(command, &output, &errOutput);
    if (ret != 0)
    {
        hlog(HLOG_DEBUG, "Command failed: (%s)", command.c_str());
        hlog(HLOG_DEBUG, "  Output: (%s)", output.c_str());
        hlog(HLOG_DEBUG, "  Error: (%s)", errOutput.c_str());
    }
    return ret;
}

int SSHRunner::Run(
    const FString &command,
    FString *output,
    FString *errorOutput)
{
    // Try to open a channel to be used for executing the command.
    LIBSSH2_CHANNEL* channel = libssh2_channel_open_session(mSession);
    if( NULL == channel )
    {
        FString stmp(FStringFC(),
                     "Could not open communication channel for executing "
                     "remote command [%s] on %s:%d.",
                     command.c_str(), mIPAddress.c_str(), mPort);
        hlog(HLOG_ERR, "%s", stmp.c_str());
        throw ERunError(stmp);
    }

    //  Execute the command.
    if (libssh2_channel_exec(channel, command.c_str()) < 0)
    {
        FString stmp(FStringFC(),
                     "Failed to run command [%s] on %s:%d (err=%s)",
                     command.c_str(), mIPAddress.c_str(), mPort,
                     getErrorString().c_str());
        hlog(HLOG_ERR, "%s", stmp.c_str());
        libssh2_channel_free(channel);
        throw ERunError(stmp);
    }

    // Read the output
    *output = "";
    int rc = 0;
    do
    {
        char buffer[0x100];
        rc = libssh2_channel_read(channel, buffer, sizeof(buffer));

        if( rc > 0 )
        {
            output->append(buffer, rc);
        }

    }
    while(rc > 0 );

    // Read the error output
    *errorOutput = "";
    do
    {
        char buffer[0x100];
        rc = libssh2_channel_read_stderr(channel, buffer, sizeof(buffer));

        if( rc > 0 )
        {
            errorOutput->append(buffer, rc);
        }

    }
    while(rc > 0 );

    // get the exitcode for the process
    int exitcode = 127;

    // Close the channel.
    if (libssh2_channel_close(channel) == 0)
    {
        exitcode = libssh2_channel_get_exit_status(channel);
    }

    // Free resources.
    libssh2_channel_free(channel);

    return exitcode;
}

void SSHRunner::GetFile(const FString &remotePath, const FString &localPath)
{
    //open the localfile to be written with scp data from remote file
    AutoFD fd(::open(localPath.c_str(), O_RDWR|O_CREAT|O_TRUNC|O_APPEND, 0644));
    if (AutoFD::NONE == fd)
    {
        char errorBuffer[256]; int errorBufSize = sizeof(errorBuffer);
        FString err(FStringFC(), "Failed to open local file %s for scp: %s",
                    localPath.c_str(),
                    strerror_r(errno, errorBuffer, errorBufSize));
        hlog(HLOG_ERR, "%s", err.c_str());
        throw ESCPError(err);
    }

    // Try to open a channel to be used for executing the command.
    struct stat statBuffer;
    memset(&statBuffer, 0, sizeof(statBuffer));
    LIBSSH2_CHANNEL* channel = libssh2_scp_recv(mSession, remotePath.c_str(), &statBuffer);
    if (NULL == channel)
    {
        FString stmp(FStringFC(),
                     "Could not open communication channel for SCP of file "
                     "[%s] on %s:%d.", remotePath.c_str(), mIPAddress.c_str(), mPort);
        hlog(HLOG_ERR, "%s", stmp.c_str());
        fd.Close();
        ::unlink(localPath.c_str());
        throw ESCPError(stmp);
    }

    //loop { read 8kB (or what is remaining) at a time from SSH peer and write it into
    //       local file } until we are done
    ssize_t got = 0;
    char readBuffer[8192];
    int readSize = sizeof(readBuffer);

    while (got < statBuffer.st_size)
    {
        if ((statBuffer.st_size - got) < readSize)
        {
            readSize = statBuffer.st_size - got;
        }
        int rc = libssh2_channel_read(channel, readBuffer, readSize);
        if (rc > 0)
        {
            ssize_t written = 0;
            while (written < rc)
            {
                ssize_t wsz = ::write(fd, readBuffer+written, rc-written);
                if (wsz < 0)
                {
                    char errorBuffer[256]; int errorBufSize = sizeof(errorBuffer);
                    FString err(FStringFC(),
                            "Failed to write local file %s for scp: %s",
                            localPath.c_str(),
                            strerror_r(errno, errorBuffer, errorBufSize));
                    hlog(HLOG_ERR, "%s", err.c_str());
                    fd.Close();
                    ::unlink(localPath.c_str());
                    libssh2_channel_free(channel);
                    channel = NULL;
                    throw ESCPError(err);
                }
                written += wsz;
            }
        }
        else if (rc < 0)
        {
            if (LIBSSH2_ERROR_EAGAIN != rc)
            {
                FString err(FStringFC(),
                            "Failed to SCP read file [%s] from %s/%d (err=%s)",
                            remotePath.c_str(), mIPAddress.c_str(), mPort,
                            getErrorString().c_str());
                hlog(HLOG_ERR, "%s", err.c_str());
                fd.Close();
                ::unlink(localPath.c_str());
                libssh2_channel_free(channel);
                channel = NULL;
                throw ESCPError(err);
            }
            else
            {
                hlog(HLOG_WARN, "Got LIBSSH2_ERROR_EAGAIN during read of [%s] from %s/%d",
                     remotePath.c_str(), mIPAddress.c_str(), mPort);
                rc = 0;
            }
        }
        //ignore (0 == rc), which is not error
        got += rc;
    }

    //cleanup
    libssh2_channel_free(channel);
    channel = NULL;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"

int SSHRunner::waitSocket(int socket_fd, LIBSSH2_SESSION *session)
{
    struct timeval timeout;
    int rc;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;
    int dir;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    FD_ZERO(&fd);

    FD_SET(socket_fd, &fd);

    /* now make sure we wait in the correct direction */
    dir = libssh2_session_block_directions(session);


    if(dir & LIBSSH2_SESSION_BLOCK_INBOUND)
        readfd = &fd;

    if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
        writefd = &fd;

    rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

    return rc;
}

int SSHRunner::createSocketAndConnect(
    const char *ipAddress, int portNumber)
{
    FTRACE2("%s, %i", ipAddress, portNumber);

    int sock = -1;

    struct sockaddr_in s;
    if (1 != inet_pton(AF_INET, ipAddress, &(s.sin_addr)))
    {
        FString stmp(FStringFC(),
                    "unable to get the in_addr for ip : %s", ipAddress);
        hlog(HLOG_ERR, "%s", stmp.c_str());
        throw ESocketError(stmp);
    }
    s.sin_family = AF_INET;
    s.sin_port = htons(portNumber);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        int theErrNo=errno;
        FString stmp(FStringFC(),
                     "Failed to create socket when connecting to %s:%d (%s)",
                     ipAddress, portNumber,
                     SystemCallUtil::GetErrorDescription(theErrNo).c_str());
        hlog(HLOG_ERR, "%s", stmp.c_str());
        throw ESocketError(stmp);
    }

    if(connect(sock, reinterpret_cast<struct sockaddr *>(&s), sizeof(s)) < 0)
    {
        int theErrNo=errno;
        close(sock);
        FString stmp;
        stmp.Format("Failed to connect with sock : %s/%d (%s)",
                    ipAddress, portNumber,
                    SystemCallUtil::GetErrorDescription(theErrNo).c_str());
        hlog(HLOG_INFO, "%s", stmp.c_str());
        throw ESocketError(stmp);
    }

    mIPAddress = ipAddress;
    mPort = portNumber;

    return sock;
}

#pragma GCC diagnostic warning "-Wold-style-cast"

FString SSHRunner::getErrorString()
{
    if (!mSession) return "NO SESSION!";
    FString errString("none");
    char* errMsg = NULL;
    int errNumber = libssh2_session_last_error(mSession, &errMsg, NULL, 1);
    if (errMsg)
    {
        errString.Format("%d:%s", errNumber, errMsg);
        free(errMsg);
    }
    return errString;
}

