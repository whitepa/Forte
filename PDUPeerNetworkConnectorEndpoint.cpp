// #SCQAD TAG: forte.pdupeer
#include <boost/make_shared.hpp>
#include "FTrace.h"
#include "SystemCallUtil.h"
#include "LogManager.h"
#include "PDUPeerNetworkConnectorEndpoint.h"
#include "Types.h"
#include <netinet/tcp.h>

using namespace boost;

Forte::PDUPeerNetworkConnectorEndpoint::PDUPeerNetworkConnectorEndpoint(
    uint64_t myPeerSetID,
    const SocketAddress& connectToAddress)
    : PDUPeerFileDescriptorEndpoint(-1),
      mPeerSetID(myPeerSetID),
      mConnectToAddress(connectToAddress)
{
    FTRACE;
}

void Forte::PDUPeerNetworkConnectorEndpoint::CheckConnection()
{
    FTRACE;

    AutoUnlockMutex lock(mConnectionMutex);

    if (GetFD() == -1)
    {
        try
        {
            connect();
        }
        catch (Exception& e)
        {
            hlogstream(HLOG_DEBUG3, "failed to connect" << e.what());
        }
    }
}

#pragma GCC diagnostic ignored "-Wold-style-cast"

void Forte::PDUPeerNetworkConnectorEndpoint::connect()
{
    FTRACE2("%s:%d",
            mConnectToAddress.first.c_str(),
            mConnectToAddress.second);

    hlogstream(HLOG_DEBUG2, "Attempting connection to "
               << mConnectToAddress.first << ":" << mConnectToAddress.second);
    AutoFD sock;
    struct sockaddr_in sa;

    if (inet_pton(
            AF_INET, mConnectToAddress.first.c_str(), &sa.sin_addr) == -1)
    {
        hlogstream(HLOG_DEBUG2,
                   "could not convert ip " << mConnectToAddress.first
                   << ":" << SystemCallUtil::GetErrorDescription(errno)
            );
        throw ECouldNotConvertIP();
    }

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        hlogstream(HLOG_DEBUG2,
                   "could not create socket " << mConnectToAddress.first
                   << ":" << SystemCallUtil::GetErrorDescription(errno)
            );
        throw ECouldNotCreateSocket();
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(mConnectToAddress.second);

    if (::connect(sock,
                  const_cast<const struct sockaddr *>(
                      reinterpret_cast<struct sockaddr*>(&sa)),
                  sizeof(sa)) == -1)
    {
        hlogstream(HLOG_DEBUG2, "could not connect to "
                   << mConnectToAddress.first
                   << ":" << mConnectToAddress.second
                   << ":" << SystemCallUtil::GetErrorDescription(errno)
            );

        throw ECouldNotConnect();
    }

    hlogstream(HLOG_DEBUG2,
               "connected to " << mConnectToAddress.first.c_str()
               << ":" << mConnectToAddress.second);

    // send identifier
    if (sizeof(mPeerSetID) != ::send(
            sock, &mPeerSetID, sizeof(mPeerSetID), MSG_NOSIGNAL))
    {
        //TODO: check for EINTR or other recoverable errors
        hlog(HLOG_DEBUG2, "could not send id to peer");
        throw ECouldNotSendID();
    }

    SetFD(sock);
    sock.Release();
    setTCPKeepAlive();

    hlog(HLOG_DEBUG, "established connection to %s:%d",
         mConnectToAddress.first.c_str(),
         mConnectToAddress.second);

}

#pragma GCC diagnostic warning "-Wold-style-cast"

void Forte::PDUPeerNetworkConnectorEndpoint::setTCPKeepAlive()
{
    FTRACE;
    static const int tcpKeepAliveEnabled(1);
    static const int tcpKeepAliveCount(4);
    static const int tcpKeepAliveIntervalSeconds(10);

    if (setsockopt(GetFD(), SOL_SOCKET, SO_KEEPALIVE,
                   &tcpKeepAliveEnabled,
                   sizeof(tcpKeepAliveEnabled)) < 0)
    {
        hlog(HLOG_WARN, "Unable to turn on TCP Keep Alive for socket"
             ", %s", SystemCallUtil::GetErrorDescription(errno).c_str());
    }
    else
    {
        hlog(HLOG_DEBUG2, "Set TCP Keep alive for socket to (%d)",
             tcpKeepAliveEnabled);

        if (setsockopt(
                GetFD(), SOL_TCP, TCP_KEEPCNT,
                &tcpKeepAliveCount,
                sizeof(tcpKeepAliveCount)) < 0)
        {
            hlog(HLOG_WARN,
                 "Unable to set on TCP_KEEPCNT for socket, using default, %s",
                 SystemCallUtil::GetErrorDescription(errno).c_str());
        }
        else
        {
            hlog(HLOG_DEBUG2, "Set TCP_KEEPCNT for socket to (%d)",
                 tcpKeepAliveCount);
        }

        if (setsockopt(
                GetFD(), SOL_TCP, TCP_KEEPINTVL,
                &tcpKeepAliveIntervalSeconds,
                sizeof(tcpKeepAliveIntervalSeconds)) < 0)
        {
            hlog(HLOG_WARN,
                 "Unable to turn on TCP_KEEPINTVL for socket, %s",
                 SystemCallUtil::GetErrorDescription(errno).c_str());
        }
        else
        {
            hlog(HLOG_DEBUG2, "Set TCP_KEEPINTVL for socket to (%d)",
                 tcpKeepAliveIntervalSeconds);
        }
    }

}

