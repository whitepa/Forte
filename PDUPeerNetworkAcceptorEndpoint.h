// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerNetworkAcceptorEndpoint_h_
#define __Forte_PDUPeerNetworkAcceptorEndpoint_h_

#include "Exception.h"
#include "Object.h"
#include "PDU.h"
#include "PDUPeerFileDescriptorEndpoint.h"
#include "Types.h"

EXCEPTION_CLASS(EPDUPeerNetworkAcceptorEndpoint);

namespace Forte
{
    class PDUPeerNetworkAcceptorEndpoint : public PDUPeerFileDescriptorEndpoint
    {
    public:
        PDUPeerNetworkAcceptorEndpoint(const SocketAddress& listenAddress);
        virtual ~PDUPeerNetworkAcceptorEndpoint() {}

        virtual void Begin() {
            //TODO: don't start listening until here.
        }
        virtual void SendPDU(const Forte::PDU &pdu);

    protected:
        SocketAddress mListenAddress;
    };

    typedef boost::shared_ptr<PDUPeerNetworkAcceptorEndpoint> PDUPeerNetworkAcceptorEndpointPtr;
};
#endif
