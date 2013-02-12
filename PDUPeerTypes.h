// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerTypes_h_
#define __Forte_PDUPeerTypes_h_

#include <boost/function.hpp>

namespace Forte
{
    class PDU;
    typedef boost::shared_ptr<PDU> PDUPtr;

    class PDUPeerEndpoint;
    typedef boost::shared_ptr<PDUPeerEndpoint> PDUPeerEndpointPtr;

    class PDUPeerNetworkConnectorEndpoint;
    typedef boost::shared_ptr<PDUPeerNetworkConnectorEndpoint> PDUPeerNetworkConnectorEndpointPtr;

    class PDUPeer;
    typedef boost::shared_ptr<PDUPeer> PDUPeerPtr;

    class PDUPeerImpl;
    typedef boost::shared_ptr<PDUPeerImpl> PDUPeerImplPtr;

    class PDUPeerSet;
    typedef boost::shared_ptr<PDUPeerSet> PDUPeerSetPtr;

    enum PDUPeerEventType
    {
        // sent when a PDU is received. Event delivered will have PDUPeer set
        PDUPeerReceivedPDUEvent,
        // sent when a PDU is past the delivery timeout. PDUPeer and
        // PDU will be valid
        PDUPeerSendErrorEvent,
        // sent when a Peer loses it's connection via the
        // endpoint. this message is informative, network endpoints
        // will still try to reconnect. PDUPeer will be set
        PDUPeerConnectedEvent,
        // sent on peer disconnect. PDUPeer field will be valid
        PDUPeerDisconnectedEvent
    };

    typedef enum {
        PDU_PEER_QUEUE_THROW,
        PDU_PEER_QUEUE_BLOCK,
        PDU_PEER_QUEUE_CALLBACK
    } PDUPeerQueueType;

    class PDUPeerEvent
    {
    public:
        PDUPeerEvent() {}
        ~ PDUPeerEvent() {}

        // types available will vary by event. see notes above.
        PDUPeerEventType mEventType;
        PDUPeerPtr mPeer;
        PDUPtr mPDU;
    };
    typedef boost::shared_ptr<PDUPeerEvent> PDUPeerEventPtr;
    typedef boost::function<void(PDUPeerEventPtr event)> PDUPeerEventCallback;

};

#endif /* __Forte_PDUPeerTypes_h_ */
