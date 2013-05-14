// #SCQAD TAG: forte.pdupeer
#include <boost/make_shared.hpp>
#include "FTrace.h"
#include "SystemCallUtil.h"
#include "PDUPeerSetBuilderImpl.h"
#include "PDUPeerSetImpl.h"
#include "PDUPeerImpl.h"
#include "PDUPeerEndpointFactoryImpl.h"

using namespace boost;

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
      mPeerSocketAddresses(peers),
      mPDUPeerEndpointFactory(new PDUPeerEndpointFactoryImpl())
{
    FTRACE2("%llu", static_cast<unsigned long long>(mID));

    hlogstream(HLOG_INFO, "My pdu peer id is " << mID
         << " based on " << listenAddress.first << ":" << listenAddress.second);

    sort(mPeerSocketAddresses.begin(), mPeerSocketAddresses.end());
    std::vector<PDUPeerPtr> pduPeers;

    foreach(const SocketAddress& a, mPeerSocketAddresses)
    {
        //TODO? this might belong in the factory
        if (listenAddress == a
            && !createInProcessPDUPeer)
        {
            continue;
        }

        PDUPeerPtr p(
            new PDUPeerImpl(
                SocketAddressToID(a),
                mPDUPeerEndpointFactory->Create(listenAddress, a, mID),
                pduPeerSendTimeout,
                queueSize,
                queueType
                ));

        pduPeers.push_back(p);
    }

    // begin setup PeerSet
    mPDUPeerSet.reset(new PDUPeerSetImpl(pduPeers));
    mPDUPeerSet->SetupEPoll();
    mPDUPeerSet->SetEventCallback(eventCallback);

    // add the pdupeerset as child to this for stats
    includeStatsFromChild(
        mPDUPeerSet,
        "PeerSet");

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
            mConnectionDispatcher,
            "PDU",
            listenAddress.second,
            32, // backlog
            listenAddress.first.c_str()));
}

Forte::PDUPeerSetBuilderImpl::PDUPeerSetBuilderImpl()
    :mID(0)
{
    std::vector<PDUPeerPtr> emptyPeerVector;

    mPDUPeerSet.reset(new PDUPeerSetImpl(emptyPeerVector));

    mPDUPeerSet->SetupEPoll();

    // add pdupeerset as child to this
    includeStatsFromChild(
        mPDUPeerSet,
        "PeerSet");
}

Forte::PDUPeerSetBuilderImpl::~PDUPeerSetBuilderImpl()
{
    FTRACE2("%llu", static_cast<unsigned long long>(mID));

    if (mReceiverThread)
    {
        mReceiverThread->Shutdown();
        mReceiverThread->WaitForShutdown();
        mReceiverThread.reset();
    }

    mPDUPeerSet->TeardownEPoll();
    mPDUPeerSet->Shutdown();
    mPDUPeerSet.reset();

    if (mConnectionDispatcher)
    {
        mConnectionDispatcher->Shutdown();
        mConnectionDispatcher.reset();
    }
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
