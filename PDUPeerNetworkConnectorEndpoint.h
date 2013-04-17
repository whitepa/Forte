// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerNetworkConnectorEndpoint_h_
#define __Forte_PDUPeerNetworkConnectorEndpoint_h_

#include "Exception.h"
#include "Object.h"
#include "Dispatcher.h"
#include "PDUPeerFileDescriptorEndpoint.h"
#include "PDUPeerTypes.h"
#include "Types.h"

namespace Forte
{
    class Mutex;
    class Event;

    class PDUPeerNetworkConnectorEndpoint : public PDUPeerFileDescriptorEndpoint
    {
    public:
        PDUPeerNetworkConnectorEndpoint(
            uint64_t ownerPeerSetID,
            const SocketAddress& connectToAddress);

        virtual ~PDUPeerNetworkConnectorEndpoint() {}

        void CheckConnection();

        boost::shared_ptr<PDUPeerNetworkConnectorEndpoint> GetPtr() {
            return boost::static_pointer_cast<PDUPeerNetworkConnectorEndpoint>(
                Object::shared_from_this());
        }

    protected:
        void connect();
        void setTCPKeepAlive();

    protected:
        mutable Mutex mConnectionMutex;
        uint64_t mPeerSetID; // sent to other peers to let them know who we are
        SocketAddress mConnectToAddress;
    };
    typedef boost::shared_ptr<PDUPeerNetworkConnectorEndpoint> PDUPeerNetworkConnectorEndpointPtr;

};
#endif
