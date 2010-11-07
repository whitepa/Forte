/* Copyright (c) 2008-2010 Scale Computing, Inc.  All Rights Reserved.
   Proprietary and Confidential */

#include "SSHRunner.h"
#include "LogManager.h"
#include <netdb.h>
#include <sys/socket.h>

using namespace Forte;

SSHRunner::SSHRunner(
    const char *username,
    const char *password,
    const char *ipAddress,
    int portNumber) :
    mSession (NULL),
    mSocket (createSocketAndConnect(ipAddress, portNumber))
{
    if (mSocket > 0)
    {
        mSession = libssh2_session_init();
        if (libssh2_session_startup(mSession, mSocket))
        {
            libssh2_session_free(mSession);
            close(mSocket);
            hlog(HLOG_ERR, "Could not startup the ssh session");
            throw ESessionError("Could not startup the ssh session");
        }

        if(libssh2_userauth_password(mSession, username, password))
        {
            libssh2_session_disconnect(mSession, "Goodbye");
            libssh2_session_free(mSession);
            close(mSocket);
            hlog(HLOG_ERR, "Could not get authenticated");
            throw ESessionError("Could not get authenticated");
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
    if (libssh2_channel_exec(channel, command.c_str()) != 0)
    {
        hlog(HLOG_ERR, "Failed to run command");
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
        hlog(HLOG_ERR, "%s", stmp.c_str());
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
        FString stmp;
        stmp.Format("Failed to connect with sock : %d", sock);
        hlog(HLOG_ERR, "%s", stmp.c_str());
        throw ESocketError(stmp);
    }

    return sock;
}
