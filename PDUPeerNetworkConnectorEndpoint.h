// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerNetworkConnectorEndpoint_h_
#define __Forte_PDUPeerNetworkConnectorEndpoint_h_

#include "Exception.h"
#include "Object.h"
#include "Dispatcher.h"
#include "PDUPeerFileDescriptorEndpoint.h"
#include "PDUPeerTypes.h"
#include "SocketUtil.h"

namespace Forte
{
    class Mutex;
    class Event;

    class PDUPeerNetworkConnectorEndpoint : public PDUPeerFileDescriptorEndpoint
    {
    public:
        PDUPeerNetworkConnectorEndpoint(
            const boost::shared_ptr<PDUQueue>& pduSendQueue,
            uint64_t ownerPeerSetID,
            const SocketAddress& connectToAddress,
            const boost::shared_ptr<EPollMonitor>& epollMonitor);

        virtual ~PDUPeerNetworkConnectorEndpoint() {}

        boost::shared_ptr<PDUPeerNetworkConnectorEndpoint> GetPtr() {
            return boost::static_pointer_cast<PDUPeerNetworkConnectorEndpoint>(
                Object::shared_from_this());
        }

    protected:
        void connect();
        uint64_t mPeerSetID; // sent to other peers to let them know who we are
        SocketAddress mConnectToAddress;
    };
    typedef boost::shared_ptr<PDUPeerNetworkConnectorEndpoint> PDUPeerNetworkConnectorEndpointPtr;

};
#endif
