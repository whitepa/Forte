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

void Forte::PDUPeerNetworkConnectorEndpoint::connect()
{
    AutoFD autosock(createInetStreamSocket());
    //setUserTimeout(autosock, 20000);
    connectToAddress(autosock, mConnectToAddress);

    // send identifier
    if (sizeof(mPeerSetID) != ::send(
            autosock, &mPeerSetID, sizeof(mPeerSetID), MSG_NOSIGNAL))
    {
        //TODO: check for EINTR or other recoverable errors
        hlog(HLOG_WARN, "could not send id to peer");
        throw ECouldNotSendID();
    }

    SetFD(autosock);
    autosock.Release();
    setTCPKeepAlive(autosock);

    hlog(HLOG_INFO, "established connection to %s:%d",
         mConnectToAddress.first.c_str(),
         mConnectToAddress.second);
}
