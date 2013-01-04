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
            DispatcherPtr dispatcher,
            const SocketAddress& connectToAddress);

        virtual ~PDUPeerNetworkConnectorEndpoint() {}

        void CheckConnections();

        /**
         * SendPDU synchronously sends the given PDU via the
         * network. throws on error.
         */
        virtual void SendPDU(const Forte::PDU &pdu);

        boost::shared_ptr<PDUPeerNetworkConnectorEndpoint> GetPtr() {
            return boost::static_pointer_cast<PDUPeerNetworkConnectorEndpoint>(
                Object::shared_from_this());
        }

    protected:
        virtual void handleFileDescriptorClose(const struct epoll_event& e);
        void connect();
        void connectOrEnqueueRetry();
        void setTCPKeepAlive();

    protected:
        mutable Mutex mConnectionMutex;
        uint64_t mPeerSetID; // sent to other peers to let them know who we are
        DispatcherPtr mDispatcher; // to queue events for periodic
                                   // connection checking
        SocketAddress mConnectToAddress;
    };
    typedef boost::shared_ptr<PDUPeerNetworkConnectorEndpoint> PDUPeerNetworkConnectorEndpointPtr;

    class CheckConnectionEvent : public PDUEvent
    {
    public:
        CheckConnectionEvent(PDUPeerNetworkConnectorEndpointPtr endpoint) :
            mEndpoint(endpoint) {}
        virtual ~CheckConnectionEvent() {}

        virtual void DoWork() {
            FTRACE;
            mEndpoint->CheckConnections();
        }
    protected:
        PDUPeerNetworkConnectorEndpointPtr mEndpoint;
    };

};
#endif
