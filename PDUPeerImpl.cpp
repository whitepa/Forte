// #SCQAD TAG: forte.pdupeer
#include "PDUPeerImpl.h"
#include "SystemCallUtil.h"
#include "LogManager.h"
#include "Types.h"
#include "PDUPeerTypes.h"
#include <boost/make_shared.hpp>

using namespace boost;

Forte::PDUPeerImpl::PDUPeerImpl(
    uint64_t connectingPeerID,
    PDUPeerEndpointPtr endpoint,
    long pduResendTimeout,
    unsigned short queueSize,
    PDUPeerQueueType queueType)
    : mConnectingPeerID(connectingPeerID),
      mEndpoint(endpoint),
      mPDUResendTimeout(pduResendTimeout),
      mPDUQueueMutex(),
      mPDUQueueNotEmptyCondition(mPDUQueueMutex),
      mQueueMaxSize(queueSize),
      mQueueType(queueType),
      mQueueSemaphore(mQueueMaxSize),
      mShutdownCalled(false)
{
    FTRACE;
    mEndpoint->SetEventCallback(
        boost::bind(&PDUPeer::PDUPeerEndpointEventCallback,
                    this,
                    _1));
}


void Forte::PDUPeerImpl::Begin()
{
    FTRACE;

    if (!mSendThread)
    {
        mSendThread.reset(
            new PDUPeerSendThread(
                boost::static_pointer_cast<Forte::PDUPeerImpl>(
                    shared_from_this())));
    }

    mEndpoint->Begin();
}

void Forte::PDUPeerImpl::Shutdown()
{
    FTRACE;

    if (mSendThread)
    {
        {
            AutoUnlockMutex lock(mPDUQueueMutex);
            mSendThread->Shutdown();
            mPDUQueueNotEmptyCondition.Signal();
        }

        mSendThread->WaitForShutdown();
        mSendThread.reset();
    }
    mShutdownCalled = true;
}

void Forte::PDUPeerImpl::SendPDU(const Forte::PDU &pdu)
{
    FTRACE;

    mEndpoint->SendPDU(pdu);
}

void Forte::PDUPeerImpl::EnqueuePDU(const Forte::PDUPtr& pdu)
{
    FTRACE;

    if (mQueueType == PDU_PEER_QUEUE_BLOCK)
        mQueueSemaphore.Wait();

    // Potential race condition between semaphore & mutex locks
    AutoUnlockMutex lock(mPDUQueueMutex);

    if (mQueueType != PDU_PEER_QUEUE_BLOCK && mPDUQueue.size()+1 > mQueueMaxSize)
    {
        switch (mQueueType)
        {
        case PDU_PEER_QUEUE_CALLBACK:
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
                hlog_and_throw(HLOG_ERR, EPDUPeerNoEventCallback());
            }
            return;
        case PDU_PEER_QUEUE_THROW:
            hlog_and_throw(HLOG_ERR, EPDUPeerQueueFull(mQueueMaxSize));
            return;
        default:
            hlog_and_throw(HLOG_ERR,
                           EPDUPeerUnknownQueueType(mQueueType));
            return;
        }
    }

    PDUHolderPtr pduHolder(new PDUHolder);
    pduHolder->enqueuedTime = mClock.GetTime();
    pduHolder->pdu = pdu;
    mPDUQueue.push_back(pduHolder);

    mPDUQueueNotEmptyCondition.Signal();
}

// called by PDUPeerSendThread
void* Forte::PDUPeerSendThread::run()
{
    FTRACE;
    hlog(HLOG_DEBUG, "Starting PDUPeerSendThread thread");
    while (!Thread::MyThread()->IsShuttingDown())
    {
        try
        {
            mPDUPeer->sendLoop();
        }
        catch (std::exception &e)
        {
            hlog(HLOG_WARN, "exception while sending PDU: %s", e.what());
        }
        catch (...)
        {
            if (hlog_ratelimit(60))
                hlog(HLOG_ERR, "Unknown exception while polling");
        }
    }
    hlogstream(HLOG_DEBUG, "Shutting down PDUPeerSendThread thread with "
               << mPDUPeer->GetQueueSize() << " PDUs in queue");
    return NULL;
}

void Forte::PDUPeerImpl::sendLoop()
{
    PDUHolderPtr pduHolder;

    {
        AutoUnlockMutex lock(mPDUQueueMutex);
        bool shuttingDown;
        while ((shuttingDown = Thread::MyThread()->IsShuttingDown()) == false
               && mPDUQueue.empty())
        {
            mPDUQueueNotEmptyCondition.Wait();
        }

        if (shuttingDown)
        {
            return;
        }

        pduHolder = mPDUQueue.front();
        mPDUQueue.pop_front();
        mQueueSemaphore.Post();
    }

    try
    {
        mEndpoint->SendPDU(*(pduHolder->pdu));
    }
    catch (Exception& e)
    {
        hlog(HLOG_DEBUG, "Could not send pdu %s", e.what());
        // TODO? re-enqueue if not expired. (not sure if that is still
        // a good idea)
        //
        // TODO: do event callbacks async
        if (mEventCallback)
        {
            PDUPeerEventPtr event(new PDUPeerEvent());
            event->mPeer = GetPtr();
            event->mEventType = PDUPeerSendErrorEvent;
            event->mPDU = pduHolder->pdu;
            mEventCallback(event);
        }
    }
}

bool Forte::PDUPeerImpl::isPDUExpired(PDUHolderPtr pduHolder)
{
    //FTRACE;

    Timespec timeout(mPDUResendTimeout, 0);
    Timespec now;
    now = mClock.GetTime();
    if (pduHolder->enqueuedTime + timeout < now)
    {
        hlogstream(HLOG_DEBUG, "list is expired, dropping");
        return true;
    }

    return false;
}

void Forte::PDUPeerImpl::emptyList()
{
    FTRACE;

    AutoUnlockMutex lock(mPDUQueueMutex);
    hlogstream(HLOG_DEBUG,
               "Error sending PDUs to peer " << mConnectingPeerID);

    PDUHolderPtr pduHolder;

    while (!mPDUQueue.empty())
    {
        if (mEventCallback)
        {
            pduHolder = mPDUQueue.front();
            PDUPeerEventPtr event(new PDUPeerEvent());
            event->mPeer = GetPtr();
            event->mEventType = PDUPeerSendErrorEvent;
            event->mPDU = pduHolder->pdu;
            mEventCallback(event);
        }
        mPDUQueue.pop_front();
        mQueueSemaphore.Post();
    }
}

bool Forte::PDUPeerImpl::IsConnected(void) const
{
    return mEndpoint->IsConnected();
}

bool Forte::PDUPeerImpl::IsPDUReady(void) const
{
    FTRACE;

    return mEndpoint->IsPDUReady();
}

bool Forte::PDUPeerImpl::RecvPDU(Forte::PDU &out)
{
    FTRACE;

    return mEndpoint->RecvPDU(out);
}
