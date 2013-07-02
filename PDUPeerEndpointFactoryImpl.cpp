// #SCQAD TAG: forte.pdupeer
#include <boost/make_shared.hpp>
#include "FTrace.h"
#include "PDUPeerEndpointFactoryImpl.h"
#include "PDUPeerFileDescriptorEndpoint.h"
#include "PDUPeerNetworkConnectorEndpoint.h"
#include "PDUPeerInProcessEndpoint.h"

using boost::shared_ptr;
using namespace Forte;

boost::shared_ptr<PDUPeerEndpoint> PDUPeerEndpointFactoryImpl::Create(
    const boost::shared_ptr<PDUQueue>& pduSendQueue,
    int fd)
{
    FTRACE2("%d", fd);
    hlog(HLOG_DEBUG2, "Creating new FileDescriptorPDUPeer");
    boost::shared_ptr<PDUPeerFileDescriptorEndpoint> p(
        new PDUPeerFileDescriptorEndpoint(
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
            new PDUPeerFileDescriptorEndpoint(
                pduSendQueue,
                mEPollMonitor.lock()));
    }
    else if (localListenSocketAddress == connectToSocketAddress)
    {
        hlog(HLOG_DEBUG2, "Creating InProcessPDUPeer for %s:%d",
             localListenSocketAddress.first.c_str(),
             localListenSocketAddress.second);
        return boost::shared_ptr<PDUPeerEndpoint>(
            new PDUPeerInProcessEndpoint(pduSendQueue));
    }
    else
    {
        hlogstream(
            HLOG_DEBUG2,
            "Creating new PDUPeerNetworkConnectorEndpoint, "
            "will maintain connection to "
            << connectToSocketAddress.first
            << ":" << connectToSocketAddress.second);

        PDUPeerNetworkConnectorEndpointPtr p(
            new PDUPeerNetworkConnectorEndpoint(
                pduSendQueue,
                outgoingPeerSetID,
                connectToSocketAddress,
                mEPollMonitor.lock()));

        return p;
    }
}
