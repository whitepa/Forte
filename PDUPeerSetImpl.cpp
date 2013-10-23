// #SCQAD TAG: forte.pdupeer
#include <sys/epoll.h>
#include <sys/types.h>
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
#include <vector>

using namespace boost;
using namespace Forte;

Forte::PDUPeerSetImpl::PDUPeerSetImpl(
    const std::vector<PDUPeerPtr>& peers,
    const boost::shared_ptr<EPollMonitor>& epollMonitor)
    : mEPollMonitor(epollMonitor)
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

    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        p.second->Start();
    }
}

void Forte::PDUPeerSetImpl::Shutdown()
{
    FTRACE;
    recordShutdownCall();

    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        p.second->Shutdown();
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
    PDUPeerEndpointFactoryImpl f(mEPollMonitor);
    boost::shared_ptr<PDUQueue> q(new PDUQueue);
    // it should be ok to use the fd as the id. any network id of a
    // pdu peer will be a very large number well above 1024
    boost::shared_ptr<PDUPeer> peer(new PDUPeerImpl(fd, f.Create(q, fd), q));
    peer->SetEventCallback(mEventCallback);

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
        peer->Shutdown();

        AutoUnlockMutex lock(mPDUPeerLock);
        mPDUPeers.erase(peer->GetID());
    }
}

void Forte::PDUPeerSetImpl::PeerAdd(const boost::shared_ptr<PDUPeer>& peer)
{
    FTRACE;

    AutoUnlockMutex lock(mPDUPeerLock);
    peer->SetEventCallback(mEventCallback);
    mPDUPeers.insert(std::make_pair(peer->GetID(), peer));

    FString peerName(FStringFC(), "Peer-%lu", peer->GetID());
    includeStatsFromChild(peer, peerName);

    if (isRunning())
    {
        peer->Start();
    }
}

/*
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
*/

void Forte::PDUPeerSetImpl::BroadcastAsync(const PDUPtr& pdu)
{
    FTRACE;

    AutoUnlockMutex lock(mPDUPeerLock);

    hlog(HLOG_DEBUG4, "will enqueue pdu on %zu peers", mPDUPeers.size());
    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        PDUPeerPtr peer(p.second);
        try
        {
            peer->EnqueuePDU(pdu);
        }
        catch(std::exception& e)
        {
            hlogstream(HLOG_DEBUG,
                       "could not enqueue pdu for " << peer->GetID());
        }
    }
}

void Forte::PDUPeerSetImpl::SetEventCallback(PDUPeerEventCallback f)
{
    FTRACE;
    AutoUnlockMutex lock(mPDUPeerLock);

    mEventCallback = f;

    foreach (const IntPDUPeerPtrPair& p, mPDUPeers)
    {
        p.second->SetEventCallback(mEventCallback);
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
