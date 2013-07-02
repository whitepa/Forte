// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerEndpointFactory_h_
#define __Forte_PDUPeerEndpointFactory_h_

#include "Exception.h"
#include "Object.h"
#include "LogManager.h"
#include "Types.h"
#include "PDUQueue.h"

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
        virtual boost::shared_ptr<PDUPeerEndpoint> Create(
            const boost::shared_ptr<PDUQueue>& pduSendQueue,
            int fd) = 0;

        /**
         * Connect to the given SocketAddress when you are also
         * listening in-process on another SocketAddress. This is
         * useful when more than one in-process class will be sharing
         * a PDUPeerSet
         *
         * @return shared_ptr to new PDUPeerEndpoint
         */
        virtual boost::shared_ptr<PDUPeerEndpoint> Create(
            const boost::shared_ptr<PDUQueue>& pduSendQueue,
            const SocketAddress& localListenSocketAddress,
            const SocketAddress& connectToSocketAddress,
            uint64_t outgoingPeerSetID) = 0;

    };
};
#endif
