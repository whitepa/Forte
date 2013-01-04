// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerEndpointFactoryImpl_h_
#define __Forte_PDUPeerEndpointFactoryImpl_h_

#include "Exception.h"
#include "Types.h"
#include "PDUPeerEndpointFactory.h"
#include "ThreadPoolDispatcher.h"

EXCEPTION_CLASS(EPDUPeerEndpointFactoryImpl);

EXCEPTION_SUBCLASS(EPDUPeerEndpointFactoryImpl,
                   EPDUPeerEndpointFactoryImplCouldNotCreate);

namespace Forte
{
    class PDUPeerEndpoint;

    class PDUPeerEndpointFactoryImpl : public PDUPeerEndpointFactory
    {
    public:
        PDUPeerEndpointFactoryImpl() {}
        virtual ~PDUPeerEndpointFactoryImpl() {}

        /**
         * Create a PDUPeerEndpoint already connected on the given fd
         *
         * @return shared_ptr to new PDUPeerEndpoint
         */
        boost::shared_ptr<PDUPeerEndpoint> Create(int fd);

        /**
         * Connect to the given SocketAddress. This is used in the
         * case where you are using a PDUPeerSet to communicate to
         * things over the network but also inside the same
         * process. if mySocketAdddress == connectToSocketAddress,
         * this will return a PDUPeerSharedMemory, otherwise, it will
         * return a PDUPeerNetworkEndpoint, either accept or connect
         * based
         *
         * @return shared_ptr to new PDUPeerEndpoint
         */
        boost::shared_ptr<PDUPeerEndpoint> Create(
            const Forte::SocketAddress& localListenSocketAddress,
            const Forte::SocketAddress& connectToSocketAddress,
            uint64_t outgoingPeerSetID,
            ThreadPoolDispatcherPtr mDispatcher);
    };
};
#endif
