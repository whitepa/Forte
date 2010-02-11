#include "ReceiverThread.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <netinet/ip.h>

// this thread just loops and accepts connections

void * CReceiverThread::run(void)
{
    // init thread name
    mThreadName.Format("%s-recv-%u", mName.c_str(), (unsigned)mThread);

    // load config
    int port, backlog;
    port = CServerMain::GetServer().mServiceConfig.GetInteger(mName + "_port");
    backlog = CServerMain::GetServer().mServiceConfig.GetInteger(mName + "_backlog");

    if (port == 0)
        throw CForteReceiverThreadException(FStringFC(),
                         "Unable to start receiver thread: no such key '%s_port' in config",
                         mName.c_str());
    if (backlog <= 0)
    {
        backlog = 32;
        hlog(HLOG_WARN, "Unable to find key '%s_backlog' in config: default is %u",
             mName.c_str(), backlog);
    }
    // create socket
    AutoFD m;
    if ((m = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        throw CForteReceiverThreadException(FStringFC(), "Failed to create socket: %s", strerror(errno));
    
    // set SO_REUSEADDR
    int one = 1;
    setsockopt(m, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // bind socket and listen
    hlog(HLOG_INFO, "Binding to port %u on %s", port,
         (mBindIP.empty() ? "all addresses" : mBindIP.c_str()));

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(struct sockaddr_in));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(port);
    if (!inet_aton(mBindIP, &(bind_addr.sin_addr)))
        throw CForteReceiverThreadException(FStringFC(), "invalid bind IP: %s", mBindIP.c_str());

    if (bind(m, (struct sockaddr *)&bind_addr, sizeof(struct sockaddr_in))==-1)
        throw CForteReceiverThreadException(FStringFC(), "failed to bind: %s", strerror(errno));

    if (listen(m, backlog)==-1)
        throw CForteReceiverThreadException(FStringFC(), "failed to listen: %s", strerror(errno));

    // set socket to close-on-exec, which prevents a bug wherein a
    // child process that restarts this server will still have this
    // socket open, which prevents us from binding those addresses
    // until the child exits, thus causing the restart to fail.

    // NOTE added 8/19/2009 PAW: the aforementioned bug is actually
    // what's supposed to happen.  This would actually be a bug in the
    // proc-runner, or whatever is doing the exec().  The correct
    // behavior is to close all file descriptors in the forked child
    // process *before* you do the exec.  We'll leave this in anyway:
    int flags = fcntl(m, F_GETFD);
    fcntl(m, F_SETFD, (long)(flags | FD_CLOEXEC));


    // loop until shutdown
    while (!mThreadShutdown)
    {
        struct sockaddr_in in_addr;
        socklen_t len;
        int s = accept(m, (struct sockaddr *)&in_addr, &len);
        if (mThreadShutdown)
            break; // TODO: we've accepted here, should we really just close it?

        if (s == -1)
        {
            // got an invalid fd, may have been interrupted
            hlog(HLOG_DEBUG, "accept failed: %s", strerror(errno));
            continue;
        }

        // TODO: shared_ptr
        CRequestEvent *e = new CRequestEvent();
        e->mFd = s;
        // mark the time of accept
        gettimeofday(&(e->mTime), NULL);
        // TODO: store client IP address in mClient
        
        // queue the event
        mDisp.enqueue(e);
    }
    return NULL;
}
