// #SCQAD TAG: forte.pdupeer
#include "PDUPeerImpl.h"
#include "SystemCallUtil.h"
#include "LogManager.h"
#include "Types.h"
#include "PDUPeerTypes.h"
#include "Thread.h"
#include <boost/make_shared.hpp>

using namespace boost;
using namespace Forte;

Forte::PDUPeerImpl::PDUPeerImpl(
    uint64_t connectingPeerID,
    PDUPeerEndpointPtr endpoint,
    const boost::shared_ptr<PDUQueue>& pduQueue)
    : mConnectingPeerID(connectingPeerID),
      mEndpoint(endpoint),
      mPDUQueue(pduQueue)
{
    FTRACE;
}

Forte::PDUPeerImpl::~PDUPeerImpl()
{
    FTRACE;
}

void Forte::PDUPeerImpl::Start()
{
    FTRACE;
    recordStartCall();

    mEndpoint->SetEventCallback(
        boost::bind(&PDUPeer::PDUPeerEndpointEventCallback,
                    this,
                    _1));
    mEndpoint->Start();

    // register the end point stats with this pdupeer as the parent
    includeStatsFromChild(mEndpoint, "Endpoint");
    includeStatsFromChild(mPDUQueue, "Queue");
}

void Forte::PDUPeerImpl::Shutdown()
{
    FTRACE;
    recordShutdownCall();

    mEndpoint->SetEventCallback(NULL);
    mEndpoint->Shutdown();

    //TODO: deal with pdu queue
    //mPDUQueue.reset();
}

/*
TODO:
void Forte::PDUPeerImpl::SendPDU(const Forte::PDU &pdu)
{
    FTRACE;
    boost::shared_ptr<PDU> pduptr(new PDU(pdu));
    EnqueuePDU(pduptr);
    //TODO: wait for pdu callback then return
}
*/
void Forte::PDUPeerImpl::EnqueuePDU(const Forte::PDUPtr& pdu)
{
    if (!mEndpoint->IsConnected()
        && mPDUQueue->GetQueueType() == PDU_PEER_QUEUE_BLOCK)
    {
        return;
    }
    try
    {
        mPDUQueue->EnqueuePDU(pdu);
    }
    catch (EPDUQueueFull& e)
    {
        if (mPDUQueue->GetQueueType() == PDU_PEER_QUEUE_CALLBACK)
        {
            if (mEventCallback)
            {
                PDUPeerEventPtr ev(new PDUPeerEvent());
                ev->mEventType = PDUPeerSendErrorEvent;
                ev->mPeer = GetPtr();
                ev->mPDU = pdu;
                mEventCallback(ev);
            }
            else
            {
                hlog_and_throw(HLOG_ERR, EPDUQueueNoEventCallback());
            }
        }
        else if (mPDUQueue->GetQueueType() == PDU_PEER_QUEUE_THROW)
        {
            throw;
        }
    }
}

void Forte::PDUPeerImpl::PDUPeerEndpointEventCallback(PDUPeerEventPtr event)
{
    if (mEventCallback)
    {
        event->mPeer = GetPtr();
        mEventCallback(event);
    }
}

bool Forte::PDUPeerImpl::IsConnected(void) const
{
    return mEndpoint->IsConnected();
}

bool Forte::PDUPeerImpl::IsPDUReady(void) const
{
    return mEndpoint->IsPDUReady();
}

bool Forte::PDUPeerImpl::RecvPDU(Forte::PDU &out)
{
    return mEndpoint->RecvPDU(out);
}
