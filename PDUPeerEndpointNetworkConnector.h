// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerEndpointNetworkConnector_h_
#define __Forte_PDUPeerEndpointNetworkConnector_h_

#include "Exception.h"
#include "Object.h"
#include "Dispatcher.h"
#include "PDUPeerEndpointFD.h"
#include "PDUPeerTypes.h"
#include "SocketUtil.h"

namespace Forte
{
    class Mutex;
    class Event;

    class PDUPeerEndpointNetworkConnector : public PDUPeerEndpointFD
    {
    public:
        PDUPeerEndpointNetworkConnector(
            const boost::shared_ptr<PDUQueue>& pduSendQueue,
            uint64_t ownerPeerSetID,
            const SocketAddress& connectToAddress,
            const boost::shared_ptr<EPollMonitor>& epollMonitor);

        virtual ~PDUPeerEndpointNetworkConnector() {}

        boost::shared_ptr<PDUPeerEndpointNetworkConnector> GetPtr() {
            return boost::static_pointer_cast<PDUPeerEndpointNetworkConnector>(
                Object::shared_from_this());
        }

    protected:
        void connect();
        uint64_t mPeerSetID; // sent to other peers to let them know who we are
        SocketAddress mConnectToAddress;
    };

};
#endif
