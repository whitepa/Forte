// #SCQAD TAG: forte.pdupeer
#include "FTrace.h"
#include "LogManager.h"
#include "PDUPeerNetworkAcceptorEndpoint.h"

Forte::PDUPeerNetworkAcceptorEndpoint::PDUPeerNetworkAcceptorEndpoint(
    const SocketAddress& listenAddress)
    : PDUPeerFileDescriptorEndpoint(-1),
      mListenAddress(listenAddress)
{
    FTRACE;
}

void Forte::PDUPeerNetworkAcceptorEndpoint::SendPDU(const Forte::PDU &pdu)
{
    FTRACE;

    //TODO? this can probably now be folded entirely into
    //PDUPeerFileDescriptorEndpoint, although we will need this when
    //we can have multiple fds, although, we may want to handle that
    //at the PDUPeerFileDescriptorEndpoint level as well
    if (mFD == -1)
        throw ENotConnected();

    PDUPeerFileDescriptorEndpoint::SendPDU(pdu);
}
