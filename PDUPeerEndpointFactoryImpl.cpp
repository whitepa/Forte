// #SCQAD TAG: forte.pdupeer
#include <boost/make_shared.hpp>
#include "FTrace.h"
#include "PDUPeerEndpointFactoryImpl.h"
#include "PDUPeerEndpointFD.h"
#include "PDUPeerEndpointNetworkConnector.h"
#include "PDUPeerEndpointInProcess.h"

using boost::shared_ptr;
using namespace Forte;

boost::shared_ptr<PDUPeerEndpoint> PDUPeerEndpointFactoryImpl::Create(
    const boost::shared_ptr<PDUQueue>& pduSendQueue,
    int fd)
{
    FTRACE2("%d", fd);
    hlog(HLOG_DEBUG2, "Creating new FileDescriptorPDUPeer");
    boost::shared_ptr<PDUPeerEndpointFD> p(
        new PDUPeerEndpointFD(
            pduSendQueue,
            mEPollMonitor.lock()));
    p->SetFD(fd);
    return p;
}

boost::shared_ptr<PDUPeerEndpoint> PDUPeerEndpointFactoryImpl::Create(
    const boost::shared_ptr<PDUQueue>& pduSendQueue,
    const SocketAddress& localListenSocketAddress,
    const SocketAddress& connectToSocketAddress,
    uint64_t outgoingPeerSetID)
{
    FTRACE2("%s:%d",
            localListenSocketAddress.first.c_str(),
            localListenSocketAddress.second);

    if (localListenSocketAddress < connectToSocketAddress)
    {
        hlogstream(
            HLOG_DEBUG2,
            "Creating new PDUPeerNetworkAcceptorEndpoint, "
            "will expect FDs passed to my PDUPeer "
            << connectToSocketAddress.first
            << ":" << connectToSocketAddress.second);

        return boost::shared_ptr<PDUPeerEndpoint>(
            new PDUPeerEndpointFD(
                pduSendQueue,
                mEPollMonitor.lock()));
    }
    else if (localListenSocketAddress == connectToSocketAddress)
    {
        hlog(HLOG_DEBUG2, "Creating InProcessPDUPeer for %s:%d",
             localListenSocketAddress.first.c_str(),
             localListenSocketAddress.second);
        return boost::shared_ptr<PDUPeerEndpoint>(
            new PDUPeerEndpointInProcess(pduSendQueue));
    }
    else
    {
        hlogstream(
            HLOG_DEBUG2,
            "Creating new PDUPeerEndpointNetworkConnector, "
            "will maintain connection to "
            << connectToSocketAddress.first
            << ":" << connectToSocketAddress.second);

        PDUPeerEndpointNetworkConnectorPtr p(
            new PDUPeerEndpointNetworkConnector(
                pduSendQueue,
                outgoingPeerSetID,
                connectToSocketAddress,
                mEPollMonitor.lock()));

        return p;
    }
}
