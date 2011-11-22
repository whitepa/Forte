#include <sys/epoll.h>
#include "PDUPeerSet.h"
#include "Foreach.h"
#include "FTrace.h"
#include "LogManager.h"
#include "Types.h"

// \TODO remove this once EPOLLRDHUP makes it into sys/epoll.h
#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

using namespace boost;
using namespace Forte;

void Forte::PDUPeerSet::PeerDelete(const shared_ptr<PDUPeer> &peer)
{
    AutoReadUnlock epollLock(mEPollLock);
    AutoUnlockMutex lock(mLock);
    if (peer)
    {
        if (mEPollFD != -1 &&
            epoll_ctl(mEPollFD, EPOLL_CTL_DEL, peer->GetFD(), NULL) < 0)
            hlog(HLOG_ERR, "EPOLL_CTL_DEL failed: %s", strerror(errno));
        // erase from FD map
        mPeerSet.erase(peer);
    }
}

shared_ptr<PDUPeer> Forte::PDUPeerSet::PeerCreate(int fd)
{
    AutoReadUnlock epollLock(mEPollLock);
    AutoUnlockMutex lock(mLock);
    shared_ptr<PDUPeer> peer(new PDUPeer(fd));
    mPeerSet.insert(peer);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLERR | EPOLLRDHUP;
    ev.data.ptr = peer.get();
    if (mEPollFD != -1 &&
        epoll_ctl(mEPollFD, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        mPeerSet.erase(peer);
        throw EPDUPeerSetPollAdd(strerror(errno));
    }
    return peer;
}

void Forte::PDUPeerSet::SendAll(const PDU &pdu) const
{
    AutoUnlockMutex lock(mLock);
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
            hlog(HLOG_WARN, "%s", e.what());
        }
    }
}

int Forte::PDUPeerSet::SetupEPoll(void)
{
    FTRACE;
    AutoWriteUnlock pollLock(mEPollLock);
    AutoFD fd;
    {
        if (mEPollFD != -1)
            return mEPollFD; // already set up
        fd = epoll_create(4);
        if (fd == -1)
            throw EPDUPeerSetPollCreate(strerror(errno));
    }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLERR | EPOLLRDHUP;

    AutoUnlockMutex lock(mLock);
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

void Forte::PDUPeerSet::TeardownEPoll(void)
{
    AutoWriteUnlock lock(mEPollLock);
    if (mEPollFD != -1)
    {
        close(mEPollFD);
        mEPollFD = -1;
        mBuffer.reset();
    }
}

void Forte::PDUPeerSet::Poll(int msTimeout, bool interruptible)
{
    int nfds = 0;
    struct epoll_event events[32];
    boost::shared_array<char> buffer;
    {
        AutoReadUnlock lock(mEPollLock);
        if (mEPollFD == -1 || !mBuffer)
            throw EPDUPeerSetNotPolling();
       
        memset(events, 0, 32*sizeof(struct epoll_event));
       
        do
        {
            nfds = epoll_wait(mEPollFD, events, 32, msTimeout);
        } 
        while (nfds < 0 && !interruptible && errno == EINTR);

        buffer = mBuffer;
    }
//    hlog(HLOG_DEBUG, "epoll_wait complete, nfds=%d", nfds);
    if (nfds < 0 && (errno == EBADF || errno == EFAULT || errno == EINVAL))
    {
        throw EPDUPeerSetPollFailed(strerror(errno));
    }
    // process the fds
    char *rawBuffer = buffer.get();
    for (int i = 0; i < nfds; ++i)
    {
//        hlog(HLOG_DEBUG, "i=%d events=0x%x", i, events[i].events);
        bool error = false;
        shared_ptr<PDUPeer> peer;
        {
            AutoUnlockMutex lock(mLock);
            PDUPeer *peerRawPtr = reinterpret_cast<PDUPeer *>(events[i].data.ptr);
            // we must validate the peer pointer in case the peer was
            // deleted prior to obtaining mLock
            // \TODO make this more efficient
            bool found = false;
            foreach(const boost::shared_ptr<PDUPeer> &p, mPeerSet)
            {
                if (p.get() == peerRawPtr)
                {
                    found = true;
                    break;
                }
            }
            if (found)
                peer = peerRawPtr->GetPtr();
            else
            {
                // this means the peer was removed after the poll but
                // before we got around to processing the received
                // data.
                hlog(HLOG_DEBUG, "peer removed before data processed; skipping");
                continue;
            }
        }
        if (events[i].events & EPOLLIN)
        {
            // input is ready
            int len = recv(peer->GetFD(), rawBuffer, RECV_BUFFER_SIZE, 0);
            if (len < 0)
            {
                hlog(HLOG_ERR, "recv failed: %s", strerror(errno));
                error = true;
            }
            else if (len == 0)
            {
                hlog(HLOG_DEBUG, "socket shutdown");
                error = true;
            }
            else
            {
                hlog(HLOG_DEBUG, "received %d bytes on fd %d", 
                     len, peer->GetFD());
                peer->DataIn(len, rawBuffer);
                if (mProcessPDUCallback && peer->IsPDUReady())
                    mProcessPDUCallback(*peer);
            }
        }
        if (error ||
            events[i].events & EPOLLERR ||
            events[i].events & EPOLLHUP ||
            events[i].events & EPOLLRDHUP)
        {
            // disconnected, or needs a disconnect
            hlog(HLOG_DEBUG, "deleting peer on fd %d", peer->GetFD());
            PeerDelete(peer);
            if (mErrorCallback)
                mErrorCallback(*peer);
            peer->Close();
        }
    }
}
