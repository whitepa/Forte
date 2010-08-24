#include <sys/epoll.h>
#include "PDUPeerSet.h"
#include "Foreach.h"
#include "LogManager.h"
#include "Types.h"

using namespace boost;
using namespace Forte;

void Forte::PDUPeerSet::PeerDelete(const shared_ptr<PDUPeer> &peer)
{
    if (peer)
    {
        // erase from FD map
        mPeerSet.erase(peer);
        if (mEPollFD != -1 &&
            epoll_ctl(mEPollFD, EPOLL_CTL_DEL, peer->GetFD(), NULL) < 0)
            hlog(HLOG_ERR, "EPOLL_CTL_DEL failed: %s", strerror(errno));
    }
}

shared_ptr<PDUPeer> Forte::PDUPeerSet::PeerCreate(int fd)
{
    shared_ptr<PDUPeer> peer(new PDUPeer(fd));
    mPeerSet.insert(peer);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | 0x2000; // \TODO EPOLLRDHUP = 0x2000
    ev.data.ptr = peer.get();
    if (mEPollFD != -1 &&
        epoll_ctl(mEPollFD, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        mPeerSet.erase(peer);
        throw EPDUPeerSetPollAdd(strerror(errno));
    }
    return peer;
}

void Forte::PDUPeerSet::SendAll(PDU &pdu)
{
    foreach (const shared_ptr<PDUPeer> &peer, mPeerSet)
    {
        if (!peer) continue;
        hlog(HLOG_DEBUG, "sending to peer on fd %d", 
             peer->GetFD());
        try
        {
            peer->SendPDU(pdu);
        }
        catch (EPeerSendFailed &e)
        {
            hlog(HLOG_WARN, "%s", e.what().c_str());
        }
    }
}

int Forte::PDUPeerSet::SetupEPoll(void)
{
    AutoFD fd = epoll_create(4);
    if (fd == -1)
        throw EPDUPeerSetPollCreate(strerror(errno));
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | 0x2000; // \TODO EPOLLRDHUP = 0x2000
    foreach (const shared_ptr<PDUPeer> &p, mPeerSet)
    {
        ev.data.ptr = p.get();
        if (epoll_ctl(fd, EPOLL_CTL_ADD, p->GetFD(), &ev) < 0)
            throw EPDUPeerSetPollAdd(strerror(errno));
    }
    mBuffer.reset(new char[RECV_BUFFER_SIZE]);
    mEPollFD = fd.Release();
    hlog(HLOG_DEBUG, "created epoll descriptor on fd %d", mEPollFD);
    return mEPollFD;
}

void Forte::PDUPeerSet::Poll(int msTimeout)
{
    if (mEPollFD == -1 || !mBuffer)
        throw EPDUPeerSetNotPolling();
    struct epoll_event events[32];
    int nfds = epoll_wait(mEPollFD, events, 32, -1);
    if (nfds < 0 &&
        ((errno == EBADF ||
          errno == EFAULT ||
          errno == EINVAL) &&
         errno != EINTR))
    {
        throw EPDUPeerSetPollFailed(strerror(errno));
    }
    // process the fds
    char *buffer = mBuffer.get();
    int len;
    for (int i = 0; i < nfds; ++i)
    {
        bool error = false;
        PDUPeer *p = reinterpret_cast<PDUPeer *>(events[i].data.ptr);
        const shared_ptr<PDUPeer> &peer(p->GetPtr());
        if (!peer)
        {
            hlog(HLOG_CRIT, "invalid peer in epoll data");
            throw EPDUPeerSetPollFailed();
        }
        if (events[i].events & EPOLLIN)
        {
            // input is ready
            if ((int)(len = recv(peer->GetFD(), buffer, RECV_BUFFER_SIZE, 0)) <= 0)
            {
                hlog(HLOG_ERR, "recv failed: %s", strerror(errno));
                error = true;
            }
            else
            {
                hlog(HLOG_DEBUG, "received %llu bytes", (u64) len);
                peer->DataIn(len, buffer);
                if (mProcessPDUCallback && peer->IsPDUReady())
                    mProcessPDUCallback(*peer);
            }
        }
        if (error ||
            events[i].events & EPOLLERR ||
            events[i].events & EPOLLHUP ||
            events[i].events & 0x2000) // \TODO EPOLLRDHUP
        {
            // disconnected, or needs a disconnect
            PeerDelete(peer);
            peer->Close();
        }
    }
}
