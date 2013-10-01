// #SCQAD TAG: forte.pdupeer
#include <boost/make_shared.hpp>
#include "FTrace.h"
#include "SystemCallUtil.h"
#include "PDUPeerSetBuilderImpl.h"
#include "PDUPeerSetImpl.h"
#include "PDUPeerImpl.h"
#include "PDUPeerEndpointFactoryImpl.h"

using namespace boost;
using namespace Forte;

Forte::PDUPeerSetBuilderImpl::PDUPeerSetBuilderImpl(
    const SocketAddress& listenAddress,
    const SocketAddressVector& peers,
    PDUPeerEventCallback eventCallback,
    int minThreads,
    int maxThreads,
    bool createInProcessPDUPeer,
    long pduPeerSendTimeout,
    unsigned short queueSize,
    PDUPeerQueueType queueType)
    : mListenAddress(listenAddress),
      mID(SocketAddressToID(listenAddress)),
      mPDUPeerSendTimeout(pduPeerSendTimeout),
      mQueueSize(queueSize),
      mQueueType(queueType),
      mEPollMonitor(new EPollMonitor("peerset-epl")),
      mPDUPeerEndpointFactory(new PDUPeerEndpointFactoryImpl(mEPollMonitor))
{
    FTRACE2("%llu", static_cast<unsigned long long>(mID));

    hlogstream(HLOG_INFO, "My pdu peer id is " << mID
         << " based on " << listenAddress.first << ":" << listenAddress.second);

    std::vector<PDUPeerPtr> pduPeers;

    foreach(const SocketAddress& sa, peers)
    {
        //TODO? this might belong in the factory
        if (listenAddress == sa
            && !createInProcessPDUPeer)
        {
            continue;
        }

        pduPeers.push_back(peerFromSocketAddress(sa));
    }

    // begin setup PeerSet
    mPDUPeerSet.reset(new PDUPeerSetImpl(pduPeers, mEPollMonitor));
    mPDUPeerSet->SetEventCallback(eventCallback);
    includeStatsFromChild(mPDUPeerSet, "PeerSet");
    // end setup PeerSet

    // begin incoming connection setup
    mConnectionHandler.reset(new PDUPeerSetConnectionHandler(mPDUPeerSet));

    mConnectionDispatcher.reset(
        new OnDemandDispatcher(
            mConnectionHandler,
            8, // max threads
            32, // deepQueue - appears to be unused here also
            32, // maxDepth
            "PDUCnect")
        );

    mReceiverThread.reset(
        new ReceiverThread(
            ReceiverThread::NoAutoStart(),
            mConnectionDispatcher,
            "PDU",
            mListenAddress.second,
            32, // backlog
            mListenAddress.first.c_str()));
}

Forte::PDUPeerSetBuilderImpl::PDUPeerSetBuilderImpl()
    : mID(0),
      mPDUPeerSendTimeout(30),
      mQueueSize(1023),
      mQueueType(PDU_PEER_QUEUE_THROW),
      mEPollMonitor(new EPollMonitor("peerset"))
{
    std::vector<PDUPeerPtr> emptyPeerVector;

    mPDUPeerSet.reset(new PDUPeerSetImpl(emptyPeerVector, mEPollMonitor));
    includeStatsFromChild(mPDUPeerSet, "PeerSet");
}

Forte::PDUPeerSetBuilderImpl::~PDUPeerSetBuilderImpl()
{
    FTRACE2("%llu", static_cast<unsigned long long>(mID));
}

void Forte::PDUPeerSetBuilderImpl::Start()
{
    recordStartCall();
    mPDUPeerSet->Start();
    mEPollMonitor->Start();

    if (mReceiverThread)
    {
        mReceiverThread->StartThread();
    }
}

void Forte::PDUPeerSetBuilderImpl::Shutdown()
{
    recordShutdownCall();

    if (mConnectionDispatcher)
    {
        mConnectionDispatcher->Shutdown();
    }

    if (mReceiverThread)
    {
        mReceiverThread->Shutdown();
    }

    mPDUPeerSet->Shutdown();

    if (mReceiverThread)
    {
        mReceiverThread->WaitForShutdown();
    }

    mEPollMonitor->Shutdown();
}

uint64_t Forte::PDUPeerSetBuilderImpl::SocketAddressToID(const SocketAddress& sa)
{
    FTRACE;
    struct sockaddr_in sa_in;
    int32_t addrAsBinary;
    uint64_t result(0);

    //TODO: need to handle the case of 0.0.0.0
    if (inet_pton(AF_INET, sa.first.c_str(), &sa_in.sin_addr) == 1)
    {
        addrAsBinary = static_cast<int32_t>(sa_in.sin_addr.s_addr);
        // switch to host ordering
        addrAsBinary = ntohl(addrAsBinary);

        //hlogstream(HLOG_DEBUG2, "addrAsBinary: " << addrAsBinary);
        // shift the id into the upper bits of
        result = addrAsBinary;
        result = result << 32;
        //hlogstream(HLOG_DEBUG2, "addrAsBinary shifted: " << result);
        //hlogstream(HLOG_DEBUG2, "port: " << sa.second);
        result = result | sa.second;
        //hlogstream(HLOG_DEBUG2, "result: " << result);
    }
    else
    {
        throw EPDUPeerSetBuilderCouldNotCreateID(
            SystemCallUtil::GetErrorDescription(errno));
    }
    return result;
}

std::string Forte::PDUPeerSetBuilderImpl::IDToSocketAddress(
    const uint64_t& peerID)
{
    FTRACE;

    uint64_t port = peerID & 0xFFFFFFFF;
    int32_t addr = peerID >> 32;
    addr = htonl(addr);

    char strAddr[INET_ADDRSTRLEN];
    struct in_addr in_addr;
    in_addr.s_addr = addr;

    if (inet_ntop(AF_INET, &in_addr, strAddr, INET_ADDRSTRLEN) != NULL)
    {
        std::stringstream s;
        s << strAddr << ":" << port;
        return s.str();
    }
    else
    {
        throw EPDUPeerSetBuilderCouldNotCreateID(
            SystemCallUtil::GetErrorDescription(errno));
    }
}

boost::shared_ptr<Forte::PDUPeer> Forte::PDUPeerSetBuilderImpl::PeerCreate(
    const Forte::SocketAddress& address)
{
    boost::shared_ptr<Forte::PDUPeer> p(peerFromSocketAddress(address));
    mPDUPeerSet->PeerAdd(p);
    return p;
}

boost::shared_ptr<Forte::PDUPeer> PDUPeerSetBuilderImpl::peerFromSocketAddress(
    const Forte::SocketAddress& peerAddress)
{
    boost::shared_ptr<PDUQueue> pduQueue(
        new Forte::PDUQueue(mPDUPeerSendTimeout, mQueueSize, mQueueType));

    return boost::shared_ptr<Forte::PDUPeer>(
        new PDUPeerImpl(
            SocketAddressToID(peerAddress),
            mPDUPeerEndpointFactory->Create(
                pduQueue, mListenAddress, peerAddress, mID),
            pduQueue));
}
