// #SCQAD TAG: forte.pdupeer
#include <boost/make_shared.hpp>
#include "FTrace.h"
#include "SystemCallUtil.h"
#include "LogManager.h"
#include "PDUPeerNetworkConnectorEndpoint.h"
#include "Types.h"
#include "SocketUtil.h"

using namespace boost;

Forte::PDUPeerNetworkConnectorEndpoint::PDUPeerNetworkConnectorEndpoint(
    const boost::shared_ptr<PDUQueue>& pduSendQueue,
    uint64_t myPeerSetID,
    const SocketAddress& connectToAddress,
    const boost::shared_ptr<EPollMonitor>& epollMonitor)
    : PDUPeerFileDescriptorEndpoint(pduSendQueue, epollMonitor),
      mPeerSetID(myPeerSetID),
      mConnectToAddress(connectToAddress)
{
    FTRACE;
}

void Forte::PDUPeerNetworkConnectorEndpoint::connect()
{
    if (GetFD() == -1)
    {
        int sock(createInetStreamSocket());
        connectToAddress(sock, mConnectToAddress);

        // send identifier
        if (sizeof(mPeerSetID) !=
            ::send(sock, &mPeerSetID, sizeof(mPeerSetID), MSG_NOSIGNAL))
        {
            //TODO: check for EINTR or other recoverable errors
            hlog(HLOG_WARN, "could not send id to peer");
            close(sock);
            return;
        }

        SetFD(sock);
        setTCPKeepAlive(sock);

        hlog(HLOG_DEBUG, "established connection to %s:%d",
             mConnectToAddress.first.c_str(),
             mConnectToAddress.second);
    }
}
