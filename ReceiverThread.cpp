#include "AutoFD.h"
#include "LogManager.h"
#include "ReceiverThread.h"
#include "SystemCallUtil.h"
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/make_shared.hpp>

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
        throw EReceiverThread(
            FStringFC(), "Failed to create socket: %s",
            SystemCallUtil::GetErrorDescription(errno).c_str());

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
        throw EReceiverThread(
            FStringFC(), "invalid bind IP: %s", mBindIP.c_str());

    if (::bind(m, (struct sockaddr*)&bind_addr, sizeof(struct sockaddr_in)) == -1)
        throw EReceiverThread(FStringFC(), "failed to bind: %s",
                              SystemCallUtil::GetErrorDescription(errno).c_str());

    if (listen(m, mBacklog) == -1)
        throw EReceiverThread(
            FStringFC(), "failed to listen: %s",
            SystemCallUtil::GetErrorDescription(errno).c_str());

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
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;
    AutoFD efd(epoll_create(2));

    if (efd == -1)
        throw EReceiverThreadPollCreate(SystemCallUtil::GetErrorDescription(errno));

    if (epoll_ctl(efd, EPOLL_CTL_ADD, m, &ev) < 0)
        throw EReceiverThreadPollAdd(SystemCallUtil::GetErrorDescription(errno));

    while (!mThreadShutdown)
    {
        struct sockaddr_in in_addr;
        socklen_t len = sizeof(in_addr);
        int s;
        struct epoll_event events[1];

        int epollResult = epoll_wait(efd, events, 1, EPOLLTIMEOUT);

        if (epollResult == 0
            || (epollResult == -1 && errno == EINTR))
        {
            //timeout with no events or interrupted
            continue;
        }
        else if (epollResult == -1)
        {
            hlog_and_throw(HLOG_DEBUG,
                           EReceiverThreadPollFailed(
                               SystemCallUtil::GetErrorDescription(errno)));
        }

        hlog(HLOG_DEBUG, "events=0x%x", events[0].events);

        if (mThreadShutdown)
        {
            break;
        }

        if (events[0].events & EPOLLIN)
        {
            s = accept(m, (struct sockaddr *)&in_addr, &len);
        }
        else
        {
            // EPOLLERR or EPOLLHUP
            if (hlog_ratelimit(3600))
                hlog(HLOG_ERR,
                     "epoll_wait returned error event %d", events[0].events);
            continue;
        }

        if (s == -1)
        {
            // got an invalid fd, may have been interrupted
            hlog(HLOG_DEBUG, "accept failed: %s",
                 SystemCallUtil::GetErrorDescription(errno).c_str());
            continue;
        }

        boost::shared_ptr<RequestEvent> e = make_shared<RequestEvent>();
        e->mFD = s;
        // mark the time of accept
        gettimeofday(&(e->mTime), NULL);
        // TODO: store client IP address in mClient

        mDisp->Enqueue(e);
    }
    return NULL;
}
