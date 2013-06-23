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

Forte::PDUPeerSetImpl::PDUPeerSetImpl(const std::vector<PDUPeerPtr>& peers)
    : mEPollFD(-1)
{
    FTRACE2("created with %zu peers", peers.size());

    foreach(const PDUPeerPtr& p, peers)
    {
        mPDUPeers[p->GetID()] = p;

        FString peerID(FStringFC(), "Peer-%lu", p->GetID());
        includeStatsFromChild(p, peerID);
    }

    mConnectedCount = ConnectedCount(this);
    registerStatVariable<0>("connectedCount", &PDUPeerSetImpl::mConnectedCount);
}

Forte::ConnectedCount::operator int64_t() {
    if (mPeerSet != NULL)
        return mPeerSet->GetConnectedCount();

    return 0;
}

Forte::PDUPeerSetImpl::~PDUPeerSetImpl()
{
    FTRACE;
}

void Forte::PDUPeerSetImpl::Start()
{
    recordStartCall();

    setupEPoll();

    mPollThread.reset(
        new FunctionThread(
            FunctionThread::AutoInit(),
            boost::bind(&PDUPeerSetImpl::runEPoll, this),
            "pdu-epoll"
            ));


    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        PDUPeerPtr peer(p.second);
        peer->Start();
    }
}

void Forte::PDUPeerSetImpl::setupEPoll()
{
    FTRACE;

    AutoFD fd;
    {
        AutoWriteUnlock pollLock(mEPollLock);
        {
            if (mEPollFD != -1)
                return; // already set up
            fd = epoll_create(4);
            if (fd == -1)
                hlog_and_throw(HLOG_WARN,
                               EPDUPeerSetPollCreate(
                                   SystemCallUtil::GetErrorDescription(errno)));

            mEPollFD = fd.Release();
        }
    }

    AutoUnlockMutex lock(mPDUPeerLock);
    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        PDUPeerPtr peer(p.second);
        peer->SetEPollFD(mEPollFD);
    }

    hlog(HLOG_DEBUG2, "created epoll descriptor on fd %d", mEPollFD);
}

void Forte::PDUPeerSetImpl::Shutdown()
{
    FTRACE;
    recordShutdownCall();

    mPollThread->Shutdown();
    mPollThread->WaitForShutdown();

    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        p.second->TeardownEPoll();
    }

    teardownEPoll();

    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        p.second->Shutdown();
    }
}

void Forte::PDUPeerSetImpl::teardownEPoll()
{
    FTRACE;

    if (mEPollFD != -1)
    {
        close(mEPollFD);
        mEPollFD = -1;
    }
}

boost::shared_ptr<PDUPeer> Forte::PDUPeerSetImpl::PeerCreate(int fd)
{
    FTRACE2("%d", fd);

    if (!isRunning())
    {
        throw EObjectNotRunning();
    }

    //TODO: pass in a factory
    PDUPeerEndpointFactoryImpl f;

    // it should be ok to use the fd as the id. any network id of a
    // pdu peer will be a very large number well above 1024
    boost::shared_ptr<PDUPeer> peer(new PDUPeerImpl(fd, f.Create(fd)));
    peer->SetEventCallback(mEventCallback);

    {
        AutoReadUnlock epollLock(mEPollLock);
        peer->SetEPollFD(mEPollFD);
    }

    {
        AutoUnlockMutex lock(mPDUPeerLock);
        mPDUPeers[peer->GetID()] = peer;

        /*TODO: re-enable. see STOR-4939
        FString peerID(FStringFC(), "Peer-%lu", peer->GetID());
        includeStatsFromChild(peer, peerID);
        */
    }
    peer->Start();

    return peer;
}

void Forte::PDUPeerSetImpl::PeerDelete(const boost::shared_ptr<PDUPeer>& peer)
{
    FTRACE;

    if (peer)
    {
        peer->TeardownEPoll();
        peer->Shutdown();
    }

    if (peer)
    {
        AutoUnlockMutex lock(mPDUPeerLock);
        mPDUPeers.erase(peer->GetID());
    }
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
            hlogstream(HLOG_WARN,
                       "Could not send pdu to peer " << peer->GetID()
                       << ": " << e.what());
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

void Forte::PDUPeerSetImpl::runEPoll()
{
    FTRACE;
    hlog(HLOG_DEBUG, "Starting poller thread");
    Thread* thisThread = Thread::MyThread();
    while (!thisThread->IsShuttingDown())
    {
        try
        {
            poll(100);
        }
        /*with this change, i believe this should no longer happen
        catch (EPDUPeerSetNotPolling& e)
        {
            if (hlog_ratelimit(20))
                hlog(HLOG_DEBUG, "PeerSetNot Polling");
            thisThread->InterruptibleSleep(Timespec::FromSeconds(1));
            }*/
        catch (EThreadPoolDispatcherShuttingDown& e)
        {
        }
        catch (std::exception &e)
        {
            hlog(HLOG_ERR,
                 "unexpected exception while polling PDUPeerSet: %s",
                 e.what());
            thisThread->InterruptibleSleep(Timespec::FromSeconds(1));
        }
        catch (...)
        {
            if (hlog_ratelimit(60))
                hlog(HLOG_ERR, "Unknown exception while polling");
            thisThread->InterruptibleSleep(Timespec::FromSeconds(1));
        }
    }
}

void Forte::PDUPeerSetImpl::poll(int msTimeout, bool interruptible)
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
            if (Thread::MyThread()->IsShuttingDown())
            {
                return;
            }
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
