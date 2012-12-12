#ifndef __Forte_PDUPeerEndpointFactory_h_
#define __Forte_PDUPeerEndpointFactory_h_

#include "Exception.h"
#include "Object.h"
#include "LogManager.h"
#include "ThreadPoolDispatcher.h"
#include "Types.h"

EXCEPTION_CLASS(EPDUPeerEndpointFactory);
EXCEPTION_SUBCLASS(EPDUPeerEndpointFactory,
                   EPDUPeerEndpointFactoryCouldNotCreate);

namespace Forte
{
    class PDUPeerEndpoint;

    class PDUPeerEndpointFactory : public Object
    {
    public:
        // herein lies maximum injection
        PDUPeerEndpointFactory() {}
        virtual ~PDUPeerEndpointFactory() {}

        /**
         * Create a PDUPeerEndpoint already connected on the given fd
         *
         * @return shared_ptr to new PDUPeerEndpoint
         */
        virtual boost::shared_ptr<PDUPeerEndpoint> Create(int fd) = 0;

        /**
         * Connect to the given SocketAddress when you are also
         * listening in-process on another SocketAddress. This is
         * useful when more than one in-process class will be sharing
         * a PDUPeerSet
         *
         * @return shared_ptr to new PDUPeerEndpoint
         */
        virtual boost::shared_ptr<PDUPeerEndpoint> Create(
            const SocketAddress& localListenSocketAddress,
            const SocketAddress& connectToSocketAddress,
            uint64_t outgoingPeerSetID,
            ThreadPoolDispatcherPtr dispatcher) = 0;

    };
};
#endif
