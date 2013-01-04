// #SCQAD TAG: forte.pdupeer
#include <boost/make_shared.hpp>
#include "FTrace.h"
#include "PDUPeerEndpointFactoryImpl.h"
#include "PDUPeerFileDescriptorEndpoint.h"
#include "PDUPeerNetworkAcceptorEndpoint.h"
#include "PDUPeerNetworkConnectorEndpoint.h"
#include "PDUPeerInProcessEndpoint.h"

using boost::shared_ptr;
using namespace Forte;

boost::shared_ptr<PDUPeerEndpoint> PDUPeerEndpointFactoryImpl::Create(int fd)
{
    FTRACE2("%d", fd);
    hlog(HLOG_DEBUG2, "Creating new FileDescriptorPDUPeer");
    return shared_ptr<PDUPeerEndpoint>(new PDUPeerFileDescriptorEndpoint(fd));
}

boost::shared_ptr<PDUPeerEndpoint> PDUPeerEndpointFactoryImpl::Create(
    const SocketAddress& localListenSocketAddress,
    const SocketAddress& connectToSocketAddress,
    uint64_t outgoingPeerSetID,
    ThreadPoolDispatcherPtr dispatcher
    )
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

        return shared_ptr<PDUPeerEndpoint>(
            new PDUPeerNetworkAcceptorEndpoint(localListenSocketAddress));
    }
    else if (localListenSocketAddress == connectToSocketAddress)
    {
        hlog(HLOG_DEBUG2, "Creating InProcessPDUPeer for %s:%d",
             localListenSocketAddress.first.c_str(),
             localListenSocketAddress.second);
        return shared_ptr<PDUPeerEndpoint>(new PDUPeerInProcessEndpoint());
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
                outgoingPeerSetID, dispatcher, connectToSocketAddress));

        p->CheckConnections();

        return p;
    }
}
