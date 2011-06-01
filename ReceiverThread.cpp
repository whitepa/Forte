#include "AutoFD.h"
#include "LogManager.h"
#include "ReceiverThread.h"
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/make_shared.hpp>
//#include <netinet/ip.h>

// \TODO remove this once EPOLLRDHUP makes it into sys/epoll.h
#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

#define EPOLLTIMEOUT 500


using namespace Forte;

// this thread just loops and accepts connections

void * Forte::ReceiverThread::run(void)
{
    // init thread name
    mThreadName.Format("%s-recv-%u", mName.c_str(), GetThreadID());

    // create socket
    AutoFD m;
    if ((m = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        throw EReceiverThread(FStringFC(), "Failed to create socket: %s", strerror(errno));
    
    // set SO_REUSEADDR
    int one = 1;
    setsockopt(m, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // bind socket and listen
    hlog(HLOG_INFO, "Binding to port %u on %s", mPort,
         (mBindIP.empty() ? "all addresses" : mBindIP.c_str()));

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(struct sockaddr_in));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(mPort);
    if (!inet_aton(mBindIP, &(bind_addr.sin_addr)))
        throw EReceiverThread(FStringFC(), "invalid bind IP: %s", mBindIP.c_str());

    if (::bind(m, (struct sockaddr *)&bind_addr, sizeof(struct sockaddr_in))==-1)
        throw EReceiverThread(FStringFC(), "failed to bind: %s", strerror(errno));

    if (listen(m, mBacklog)==-1)
        throw EReceiverThread(FStringFC(), "failed to listen: %s", strerror(errno));

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


    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLERR | EPOLLRDHUP;
    ev.data.ptr = NULL;
    AutoFD efd(epoll_create(2));

    if (efd == -1)
        throw EReceiverThreadPollCreate(strerror(errno));

    if (epoll_ctl(efd, EPOLL_CTL_ADD, m, &ev) < 0)
                throw ERecieverThreadPollAdd(strerror(errno));


    // loop until shutdown
    while (!mThreadShutdown)
    {
        struct sockaddr_in in_addr;
        socklen_t len = sizeof(in_addr);
        int s;
        struct epoll_event events[1];

        int epollResult = epoll_wait(efd,events,1,EPOLLTIMEOUT);

        if (epollResult == 0)
        {
                continue;
        }
        else
        {
            if (epollResult < 0 &&
                    ((errno == EBADF ||
                      errno == EFAULT ||
                      errno == EINVAL) &&
                     errno != EINTR))
            {
                throw EReceiverThreadPollFailed(strerror(errno));
            }
        }

        if (mThreadShutdown)
            break;
        else
            s = accept(m, (struct sockaddr *)&in_addr, &len);


        if (s == -1)
        {
            // got an invalid fd, may have been interrupted
            hlog(HLOG_DEBUG, "accept failed: %s", strerror(errno));
            continue;
        }

        // TODO: shared_ptr
        boost::shared_ptr<RequestEvent> e = make_shared<RequestEvent>();
        e->mFD = s;
        // mark the time of accept
        gettimeofday(&(e->mTime), NULL);
        // TODO: store client IP address in mClient
        
        // queue the event
        mDisp->Enqueue(e);
    }
    return NULL;
}
