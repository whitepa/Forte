// #SCQAD TAG: forte.pdupeer
#include <boost/make_shared.hpp>
#include "PDUPeerImpl.h"
#include "PDUPeerSetImpl.h"
#include "PDUPeerEndpointFactoryImpl.h"
#include "Foreach.h"
#include "FTrace.h"
#include "LogManager.h"
#include "SystemCallUtil.h"
#include "Types.h"
#include "SystemCallUtil.h"

using namespace boost;
using namespace Forte;

Forte::PDUPeerSetImpl::PDUPeerSetImpl(
    DispatcherPtr dispatcher,
    PDUPeerSetWorkHandlerPtr workHandler,
    const std::vector<PDUPeerPtr>& peers)
    : mWorkDispatcher(dispatcher),
      mWorkHandler(workHandler),
      mEPollFD(-1)
{
    FTRACE2("created with %zu peers", peers.size());

    foreach(const PDUPeerPtr& p, peers)
    {
        mPDUPeers[p->GetID()] = p;
    }
}

void Forte::PDUPeerSetImpl::StartPolling()
{
    mPollThread.reset(new PDUPollThread(*this));

    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        PDUPeerPtr peer(p.second);
        peer->Begin();
    }
}

Forte::PDUPeerSetImpl::~PDUPeerSetImpl()
{
    FTRACE;

    try
    {
        if (mPollThread)
        {
            mPollThread->Shutdown();
            mPollThread->WaitForShutdown();
            mPollThread.reset();
        }

        TeardownEPoll();

        mWorkDispatcher->Shutdown();
        mWorkDispatcher.reset();
    }
    catch (Exception& e)
    {
        hlog(HLOG_WARN, "Could not clean up epoll fd or "
             "it was already closed");
    }
}

void Forte::PDUPeerSetImpl::PeerDelete(const shared_ptr<PDUPeer>& peer)
{
    FTRACE;
    AutoReadUnlock epollLock(mEPollLock);
    AutoUnlockMutex lock(mPDUPeerLock);

    if (peer)
    {
        peer->TeardownEPoll();
        mPDUPeers.erase(peer->GetID());
    }
}

shared_ptr<PDUPeer> Forte::PDUPeerSetImpl::PeerCreate(int fd)
{
    FTRACE2("%d", fd);
    AutoReadUnlock epollLock(mEPollLock);
    AutoUnlockMutex lock(mPDUPeerLock);

    //TODO: pass in a factory
    PDUPeerEndpointFactoryImpl f;
    // it should be ok to use the fd as the id. any network id of a
    // pdu peer will be a very large number well above 1024
    shared_ptr<PDUPeer> peer(new PDUPeerImpl(fd, mWorkDispatcher, f.Create(fd)));
    mPDUPeers[peer->GetID()] = peer;
    peer->SetEventCallback(mEventCallback);
    peer->SetEPollFD(mEPollFD);

    return peer;
}

void Forte::PDUPeerSetImpl::Broadcast(const PDU& pdu) const
{
    FTRACE;
    AutoUnlockMutex lock(mPDUPeerLock);

    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        PDUPeerPtr peer(p.second);
        hlogstream(HLOG_DEBUG2, "sending to peer" << peer->GetID());

        try
        {
            peer->SendPDU(pdu);
        }
        catch (Exception& e)
        {
            //TODO: determine if the error callback should be called here
            hlogstream(HLOG_WARN, "Could not send pdu to peer" << peer->GetID());
        }
    }
}

void Forte::PDUPeerSetImpl::BroadcastAsync(const PDUPtr& pdu)
{
    FTRACE;

    AutoUnlockMutex lock(mPDUPeerLock);

    hlog(HLOG_DEBUG2, "will enqueue pdu on %zu peers", mPDUPeers.size());
    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        PDUPeerPtr peer(p.second);
        peer->EnqueuePDU(pdu);
    }
}

int Forte::PDUPeerSetImpl::SetupEPoll()
{
    FTRACE;
    AutoWriteUnlock pollLock(mEPollLock);
    AutoFD fd;
    {
        if (mEPollFD != -1)
            return mEPollFD; // already set up
        fd = epoll_create(4);
        if (fd == -1)
            hlog_and_throw(HLOG_WARN,
                           EPDUPeerSetPollCreate(
                               SystemCallUtil::GetErrorDescription(errno)));
    }

    AutoUnlockMutex lock(mPDUPeerLock);
    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        PDUPeerPtr peer(p.second);
        peer->SetEPollFD(fd);
    }
    mEPollFD = fd.Release();
    hlog(HLOG_DEBUG2, "created epoll descriptor on fd %d", mEPollFD);
    return mEPollFD;
}

void Forte::PDUPeerSetImpl::TeardownEPoll()
{
    FTRACE;

    AutoWriteUnlock lock(mEPollLock);
    AutoUnlockMutex peerlock(mPDUPeerLock);

    if (mEPollFD != -1)
    {
        foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
        {
            PDUPeerPtr peer(p.second);
            peer->TeardownEPoll();
        }

        close(mEPollFD);
        mEPollFD = -1;
    }
}

void Forte::PDUPeerSetImpl::SetEventCallback(PDUPeerEventCallback f)
{
    FTRACE;
    AutoUnlockMutex lock(mPDUPeerLock);

    mEventCallback = f;

    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        PDUPeerPtr peer(p.second);
        if (!peer)
        {
            hlog(HLOG_DEBUG2, "peer disappeared while looping through peerset");
            continue;
        }
        peer->SetEventCallback(mEventCallback);
    }
}

void Forte::PDUPeerSetImpl::Poll(int msTimeout, bool interruptible)
{
    int fdReadyCount = 0;
    struct epoll_event events[32];

    {
        AutoReadUnlock lock(mEPollLock);
        if (mEPollFD == -1)
            throw EPDUPeerSetNotPolling();

        memset(events, 0, 32*sizeof(struct epoll_event));

        do
        {
            fdReadyCount = epoll_wait(mEPollFD, events, 32, msTimeout);
        }
        while (fdReadyCount < 0 && !interruptible && errno == EINTR);
    }

    //hlog(HLOG_DEBUG2, "epoll_wait complete, fdReadyCount=%d", fdReadyCount);
    if (fdReadyCount < 0
        && (errno == EBADF || errno == EFAULT || errno == EINVAL))
    {
        throw EPDUPeerSetPollFailed(
            SystemCallUtil::GetErrorDescription(errno));
    }

    // process the fds
    for (int i = 0; i < fdReadyCount; ++i)
    {
        hlog(HLOG_DEBUG2, "i=%d events=0x%x, fd=%d",
             i, events[i].events, events[i].data.fd);

        PDUPeerPtr peer;
        {
            AutoUnlockMutex lock(mPDUPeerLock);

            foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
            {
                peer = p.second;
                if (peer->OwnsFD(events[i].data.fd))
                {
                    hlog(HLOG_DEBUG2, "found a peer to handle epoll event");
                    break;
                }
            }
        }

        if (peer)
        {
            //TODO: enqueue this event rather than process directly.
            try
            {
                peer->HandleEPollEvent(events[i]);
            }
            catch (Exception& e)
            {
                hlog(HLOG_ERR, "unhandled error polling event %s",
                     e.what());
            }
        }
        else
        {
            hlog(HLOG_DEBUG2, "could not find peer to handle epoll event");
        }
    }
}

unsigned int Forte::PDUPeerSetImpl::GetConnectedCount()
{
    AutoUnlockMutex lock(mPDUPeerLock);
    unsigned int connectedCount(0);
    PDUPeerPtr peer;

    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        peer = p.second;
        if (peer->IsConnected())
        {
            connectedCount++;
        }
    }

    return connectedCount;
}
