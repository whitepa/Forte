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
    DispatcherPtr dispatcher,
    const SocketAddress& connectToAddress)
    : PDUPeerFileDescriptorEndpoint(-1),
      mPeerSetID(myPeerSetID),
      mDispatcher(dispatcher),
      mConnectToAddress(connectToAddress)
{
    FTRACE;
}

void Forte::PDUPeerNetworkConnectorEndpoint::CheckConnections()
{
    FTRACE;

    AutoUnlockMutex lock(mConnectionMutex);
    connectOrEnqueueRetry();
}

void Forte::PDUPeerNetworkConnectorEndpoint::connectOrEnqueueRetry()
{
    if (GetFD() == -1)
    {
        try
        {
            connect();
        }
        catch (Exception& e)
        {
            hlogstream(HLOG_DEBUG2, "could not connect, enqueueing event");
            try
            {
                // only retry every second, seems often enough
                const Timespec onesec = Timespec::FromSeconds(1);
                Thread::InterruptibleSleep(onesec);
            }
            catch (EThreadUnknown& e)
            {
                // this will occur if you are using this class in a
                // single thread program (probably a unit test)
                sleep(1);
            }

            mDispatcher->Enqueue(
                boost::make_shared<CheckConnectionEvent>(GetPtr()));
        }
    }
}

void Forte::PDUPeerNetworkConnectorEndpoint::fileDescriptorClosed()
{
    FTRACE;
    mDispatcher->Enqueue(boost::make_shared<CheckConnectionEvent>(GetPtr()));
}

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

    if (::connect(sock, (const struct sockaddr*) &sa, sizeof(sa)) == -1)
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

void Forte::PDUPeerNetworkConnectorEndpoint::SendPDU(const Forte::PDU &pdu)
{
    FTRACE;
    AutoUnlockMutex lock(mConnectionMutex);

    try
    {
        if (GetFD() == -1)
        {
            connect();
        }

        PDUPeerFileDescriptorEndpoint::SendPDU(pdu);
    }
    catch (EPDUPeerEndpoint &e)
    {
        hlogstream(HLOG_DEBUG2, "Couldn't connect or lost connect: " << e.what());
        mDispatcher->Enqueue(boost::make_shared<CheckConnectionEvent>(GetPtr()));
        throw;
    }
}
