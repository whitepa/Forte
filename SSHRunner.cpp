/* Copyright (c) 2008-2010 Scale Computing, Inc.  All Rights Reserved.
   Proprietary and Confidential */

#include "SSHRunner.h"
#include "LogManager.h"
#include <netdb.h>
#include <sys/socket.h>
#include "FTrace.h"

using namespace Forte;

SSHRunner::SSHRunner(
    const char *username,
    const char *password,
    const char *ipAddress,
    int portNumber) :
    mSession (NULL),
    mSocket (createSocketAndConnect(ipAddress, portNumber))
{
    int err;

    if (mSocket > 0)
    {
        mSession = libssh2_session_init();
        if ((err=libssh2_session_startup(mSession, mSocket)))
        {
            libssh2_session_free(mSession);
            close(mSocket);
            hlog(HLOG_ERR, "Could not startup the ssh session (%s)", 
                 getErrorString(err).c_str());
            throw ESessionError("Could not startup the ssh session");
        }

        if ((err=libssh2_userauth_password(mSession, username, password)))
        {
            libssh2_session_disconnect(mSession, "Goodbye");
            libssh2_session_free(mSession);
            close(mSocket);
            hlog(HLOG_ERR, "Could not get authenticated (%s)",
                 getErrorString(err).c_str());
            throw ESessionError("Could not get authenticated");
        }
    }
    else
    {
        hlog(HLOG_ERR, "No Socket for creating ssh session");
        throw ESessionError("No Socket for creating ssh session");
    }
}

SSHRunner::SSHRunner(const char *username,
                     const char *publicKeyFilePath,
                     const char *privateKeyFilePath,
                     const char *passphrase,
                     const char *ipAddress,
                     int portNumber) :
    mSession (NULL),
    mSocket (createSocketAndConnect(ipAddress, portNumber))
{
    FTRACE2("%s, %s, %s, %s, %s, %i", username, publicKeyFilePath, 
            privateKeyFilePath, passphrase, ipAddress, portNumber);
    int err;

    if (mSocket > 0)
    {
        mSession = libssh2_session_init();
        if ((err=libssh2_session_startup(mSession, mSocket)))
        {
            libssh2_session_free(mSession);
            close(mSocket);
            hlog(HLOG_ERR, "Could not startup the ssh session (%s)",
                 getErrorString(err).c_str());
            throw ESessionError("Could not startup the ssh session");
        }

        if ((err=libssh2_userauth_publickey_fromfile_ex(mSession, username, 
                                                        strlen(username),
                                                        publicKeyFilePath,
                                                        privateKeyFilePath,
                                                        passphrase)))
            {
                libssh2_session_disconnect(mSession, "Goodbye");
                libssh2_session_free(mSession);
                close(mSocket);
                hlog(HLOG_ERR, "Could not get authenticated via public key (%s)",
                     getErrorString(err).c_str());
                throw ESessionError("Could not get authenticated via public key");
            }
    }
    else
    {
        hlog(HLOG_ERR, "No Socket for creating ssh session");
        throw ESessionError("No Socket for creating ssh session");
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

int SSHRunner::Run(
    const FString &command, 
    FString *output,
    FString *errorOutput)
{
    int err;
    // Try to open a channel to be used for executing the command.
    LIBSSH2_CHANNEL* channel = libssh2_channel_open_session(mSession);
    if( NULL == channel )
    {
        hlog(HLOG_ERR, "Could not open communication channel for "
             "executing remote command.");
        throw ERunError("Could not open communication channel for "
                        "executing remote command.");
    }

    //  Execute the command.
    if ((err=libssh2_channel_exec(channel, command.c_str()) != 0))
    {
        hlog(HLOG_ERR, "Failed to run command [%s] (%s)", command.c_str(),
            getErrorString(err).c_str());
        throw ERunError("Failed to run command");
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

unsigned int SSHRunner::createSocketAndConnect(
    const char *ipAddress, int portNumber)
{
    int sock = -1;
    in_addr_t data;
    data = inet_addr(ipAddress);
    struct hostent* hp = gethostbyaddr(&data, 4, AF_INET);
    if(!hp)
    {
        FString stmp;
        stmp.Format("could not get hostbyname for %s", ipAddress);
        hlog(HLOG_ERR, "%s (%i: %s)", stmp.c_str(), h_errno, 
             hstrerror(h_errno));
        throw ESocketError(stmp);
    }

    struct sockaddr_in s;
    s.sin_addr = *(struct in_addr *)hp->h_addr_list[0];
    s.sin_family = hp->h_addrtype;
    s.sin_port = htons(portNumber);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        hlog(HLOG_ERR, "Failed to create socket");
        throw ESocketError("Failed to create socket");
    }

    if(connect(sock, (struct sockaddr *)&s, sizeof(s)) < 0)
    {
        close(sock);
        FString stmp;
        stmp.Format("Failed to connect with sock : %d", sock);
        hlog(HLOG_ERR, "%s", stmp.c_str());
        throw ESocketError(stmp);
    }

    return sock;
}

FString SSHRunner::getErrorString(int errNumber)
{
    FString errString;
    switch (errNumber)
    {
    case LIBSSH2_ERROR_SOCKET_NONE:
        errString="LIBSSH2_ERROR_SOCKET_NONE";
        break;
    case LIBSSH2_ERROR_BANNER_NONE:
        errString="LIBSSH2_ERROR_BANNER_NONE";
        break;
    case LIBSSH2_ERROR_BANNER_SEND:
        errString="LIBSSH2_ERROR_BANNER_SEND";
        break;
    case LIBSSH2_ERROR_INVALID_MAC:
        errString="LIBSSH2_ERROR_INVALID_MAC";
        break;
    case LIBSSH2_ERROR_KEX_FAILURE:
        errString="LIBSSH2_ERROR_KEX_FAILURE";
        break;
    case LIBSSH2_ERROR_ALLOC:
        errString="An internal memory allocation call failed.";
        break;
    case LIBSSH2_ERROR_SOCKET_SEND:
        errString="Unable to send data on socket.";
        break;
    case LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE:
        errString="LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE";
        break;
    case LIBSSH2_ERROR_TIMEOUT:
        errString="LIBSSH2_ERROR_TIMEOUT";
        break;
    case LIBSSH2_ERROR_HOSTKEY_INIT:
        errString="LIBSSH2_ERROR_HOSTKEY_INIT";
        break;
    case LIBSSH2_ERROR_HOSTKEY_SIGN:
        errString="LIBSSH2_ERROR_HOSTKEY_SIGN";
        break;
    case LIBSSH2_ERROR_DECRYPT:
        errString="LIBSSH2_ERROR_DECRYPT";
        break;
    case LIBSSH2_ERROR_SOCKET_DISCONNECT:
        errString="LIBSSH2_ERROR_SOCKET_DISCONNECT";
        break;
    case LIBSSH2_ERROR_PROTO:
        errString="LIBSSH2_ERROR_PROTO";
        break;
    case LIBSSH2_ERROR_PASSWORD_EXPIRED:
        errString="LIBSSH2_ERROR_PASSWORD_EXPIRED";
        break;
    case LIBSSH2_ERROR_FILE:
        errString="LIBSSH2_ERROR_FILE";
        break;
    case LIBSSH2_ERROR_METHOD_NONE:
        errString="LIBSSH2_ERROR_METHOD_NONE";
        break;
    case LIBSSH2_ERROR_AUTHENTICATION_FAILED:
        errString="Authentication using the supplied password or public key was not accepted.";
        break;
    case LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED:
        errString="The username/public key combination was invalid.";
        break;
    case LIBSSH2_ERROR_CHANNEL_OUTOFORDER:
        errString="LIBSSH2_ERROR_CHANNEL_OUTOFORDER";
        break;
    case LIBSSH2_ERROR_CHANNEL_FAILURE:
        errString="LIBSSH2_ERROR_CHANNEL_FAILURE";
        break;
    case LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED:
        errString="LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED";
        break;
    case LIBSSH2_ERROR_CHANNEL_UNKNOWN:
        errString="LIBSSH2_ERROR_CHANNEL_UNKNOWN";
        break;
    case LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED:
        errString="LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED";
        break;
    case LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED:
        errString="LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED";
        break;
    case LIBSSH2_ERROR_CHANNEL_CLOSED:
        errString="LIBSSH2_ERROR_CHANNEL_CLOSED";
        break;
    case LIBSSH2_ERROR_CHANNEL_EOF_SENT:
        errString="LIBSSH2_ERROR_CHANNEL_EOF_SENT";
        break;
    case LIBSSH2_ERROR_SCP_PROTOCOL:
        errString="LIBSSH2_ERROR_SCP_PROTOCOL";
        break;
    case LIBSSH2_ERROR_ZLIB:
        errString="LIBSSH2_ERROR_ZLIB";
        break;
    case LIBSSH2_ERROR_SOCKET_TIMEOUT:
        errString="Socket timeout";
        break;
    case LIBSSH2_ERROR_SFTP_PROTOCOL:
        errString="LIBSSH2_ERROR_SFTP_PROTOCOL";
        break;
    case LIBSSH2_ERROR_REQUEST_DENIED:
        errString="LIBSSH2_ERROR_REQUEST_DENIED";
        break;
    case LIBSSH2_ERROR_METHOD_NOT_SUPPORTED:
        errString="LIBSSH2_ERROR_METHOD_NOT_SUPPORTED";
        break;
    case LIBSSH2_ERROR_INVAL:
        errString="LIBSSH2_ERROR_INVAL";
        break;
    case LIBSSH2_ERROR_INVALID_POLL_TYPE:
        errString="LIBSSH2_ERROR_INVALID_POLL_TYPE";
        break;
    case LIBSSH2_ERROR_PUBLICKEY_PROTOCOL:
        errString="LIBSSH2_ERROR_PUBLICKEY_PROTOCOL";
        break;
    case LIBSSH2_ERROR_EAGAIN:
        errString="LIBSSH2_ERROR_EAGAIN";
        break;
    case LIBSSH2_ERROR_BUFFER_TOO_SMALL:
        errString="LIBSSH2_ERROR_BUFFER_TOO_SMALL";
        break;
    case LIBSSH2_ERROR_BAD_USE:
        errString="LIBSSH2_ERROR_BAD_USE";
        break;
    case LIBSSH2_ERROR_COMPRESS:
        errString="LIBSSH2_ERROR_COMPRESS";
        break;
    case LIBSSH2_ERROR_OUT_OF_BOUNDARY:
        errString="LIBSSH2_ERROR_OUT_OF_BOUNDARY";
        break;
    case LIBSSH2_ERROR_AGENT_PROTOCOL:
        errString="LIBSSH2_ERROR_AGENT_PROTOCOL";
        break;
    default:
        errString="Unknown Error";
        break;
    }

    return errString;
}

