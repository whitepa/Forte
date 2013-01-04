// #SCQAD TAG: forte.pdupeer
#include "PDUPeerSetConnectionHandler.h"
#include "SystemCallUtil.h"
#include "FTrace.h"

using namespace Forte;
using namespace boost;

PDUPeerSetConnectionHandler::PDUPeerSetConnectionHandler(
    const PDUPeerSetPtr& peerSet)
    : RequestHandler(0),
      mPDUPeerSet(peerSet)
{
    FTRACE;
}

PDUPeerSetConnectionHandler::~PDUPeerSetConnectionHandler()
{
    FTRACE;
}

void PDUPeerSetConnectionHandler::Handler(Forte::Event* e)
{
    FTRACE;
    RequestEvent* event = dynamic_cast<RequestEvent*>(e);
    if (event == NULL)
    {
        throw std::bad_cast();
    }
    hlogstream(HLOG_DEBUG2, "Incoming connection on " << event->mFD);

    //TODO: handle interrupted syscall
    uint64_t id;
    int len = recv(event->mFD, &id, sizeof(id), 0);
    if (len < 0)
    {
        hlog(HLOG_WARN, "recv failed: %s",
             SystemCallUtil::GetErrorDescription(errno).c_str());
        close(event->mFD);
    }
    else if (len == 0)
    {
        hlog(HLOG_DEBUG2, "socket shutdown");
        close(event->mFD);
    }
    else if (len != sizeof(id))
    {
        //TODO: just read some more
        hlog(HLOG_DEBUG2, "received %d bytes on fd %d", len, event->mFD);
        close(event->mFD);
    }
    else
    {
        //TODO: convert id to ip:socket for logging purposes
        hlogstream(HLOG_DEBUG2, "received connect from peer " << id);
        mPDUPeerSet->PeerAddFD(id, event->mFD);
    }
}
