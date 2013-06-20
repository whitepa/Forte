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
    long pduSendTimeout,
    unsigned short queueSize,
    PDUPeerQueueType queueType)
    : mConnectingPeerID(connectingPeerID),
      mEndpoint(endpoint),
      mPDUSendTimeout(pduSendTimeout),
      mPDUQueueMutex(),
      mPDUQueueNotEmptyCondition(mPDUQueueMutex),
      mQueueMaxSize(queueSize),
      mQueueType(queueType),
      mQueueSemaphore(mQueueMaxSize),
      // init all the stat variables
      mTotalSent (0),
      mTotalReceived (0),
      mTotalQueued (0),
      mSendErrors (0),
      mQueueSize (0),
      mStartTime (0)
{
    FTRACE;
    mEndpoint->SetEventCallback(
        boost::bind(&PDUPeer::PDUPeerEndpointEventCallback,
                    this,
                    _1));

    registerStatVariable<0>("totalSent", &PDUPeerImpl::mTotalSent);
    registerStatVariable<1>("totalReceived", &PDUPeerImpl::mTotalReceived);
    registerStatVariable<2>("totalQueued", &PDUPeerImpl::mTotalQueued);
    registerStatVariable<3>("sendErrors", &PDUPeerImpl::mSendErrors);
    registerStatVariable<4>("queueSize", &PDUPeerImpl::mQueueSize);
    registerStatVariable<5>("startTime", &PDUPeerImpl::mStartTime);
    registerStatVariable<6>("averageQueueSize", &PDUPeerImpl::mAvgQueueSize);
}

Forte::PDUPeerImpl::~PDUPeerImpl()
{
    FTRACE;
}

void Forte::PDUPeerImpl::Start()
{
    FTRACE;
    recordStartCall();

    // register the end point stats with this pdupeer as the parent
    includeStatsFromChild(mEndpoint, "Endpoint");

    mSendThread.reset(
        new FunctionThread(
            FunctionThread::AutoInit(),
            boost::bind(&PDUPeerImpl::sendThreadRun, this),
            "pdusender"));

    mStartTime = mClock.GetTime().AsSeconds();
}

void Forte::PDUPeerImpl::Shutdown()
{
    FTRACE;
    recordShutdownCall();

    {
        AutoUnlockMutex lock(mPDUQueueMutex);
        mSendThread->Shutdown();
        mPDUQueueNotEmptyCondition.Signal();
    }

    mSendThread->WaitForShutdown();
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

    mTotalQueued++;
    mQueueSize = mPDUQueue.size();
    mAvgQueueSize = mPDUQueue.size();

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

void Forte::PDUPeerImpl::sendThreadRun()
{
    FTRACE;
    hlog(HLOG_DEBUG, "Starting PDUPeerSendThread thread");
    Forte::Thread* myThread = Thread::MyThread();
    while (!myThread->IsShuttingDown())
    {
        try
        {
            sendLoop();
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
               << GetQueueSize() << " PDUs in queue");
}

void Forte::PDUPeerImpl::sendLoop()
{
    PDUHolderPtr pduHolder;

    //to get around a lock ordering issue, it is easier to just check
    //IsConnected outside of mPDUQueueMutex
    bool endpointIsConnected = mEndpoint->IsConnected();

    {
        // this loop is messy but does what we need it to. this thread
        // will sleep while it is connected and there are no PDUs to
        // send. it will periodically ensure the connection is up.
        // when there PDUs to send it will do that.
        AutoUnlockMutex lock(mPDUQueueMutex);
        while (mPDUQueue.empty() || !endpointIsConnected)
        {
            if (Thread::MyThread()->IsShuttingDown())
            {
                return;
            }

            {
                AutoLockMutex unlock(mPDUQueueMutex);
                mEndpoint->CheckConnection();
                endpointIsConnected = mEndpoint->IsConnected();
                if (!endpointIsConnected)
                {
                    failExpiredPDUs();
                }
            }
            mQueueSize = mPDUQueue.size();
            mAvgQueueSize = mPDUQueue.size();
            mPDUQueueNotEmptyCondition.TimedWait(1);
        }

        pduHolder = mPDUQueue.front();
        mPDUQueue.pop_front();
        mQueueSemaphore.Post();
    }

    try
    {
        mEndpoint->SendPDU(*(pduHolder->pdu));
        mTotalSent++;
    }
    catch (Exception& e)
    {
        if (hlog_ratelimit(60))
        {
            hlog(HLOG_DEBUG, "Could not send pdu %s", e.what());
        }
        else
        {
            hlog(HLOG_DEBUG4, "Could not send pdu %s", e.what());
        }
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
        mSendErrors++;
    }
}

bool Forte::PDUPeerImpl::isPDUExpired(PDUHolderPtr pduHolder)
{
    //FTRACE;

    Timespec timeout(mPDUSendTimeout, 0);
    Timespec now;
    now = mClock.GetTime();
    if (pduHolder->enqueuedTime + timeout < now)
    {
        return true;
    }

    return false;
}

void Forte::PDUPeerImpl::failExpiredPDUs()
{
    FTRACE;

    std::vector<PDUPeerEventPtr> events;

    {
        AutoUnlockMutex lock(mPDUQueueMutex);
        PDUHolderPtr pduHolder;
        // newest items will be at the back. loop until we find one that
        // is not expired or the list is emtpy
        while (!mPDUQueue.empty() && isPDUExpired(mPDUQueue.front()))
        {
            pduHolder = mPDUQueue.front();
            if (isPDUExpired(pduHolder))
            {
                if (mEventCallback)
                {
                    PDUPeerEventPtr event(new PDUPeerEvent());
                    event->mPeer = GetPtr();
                    event->mEventType = PDUPeerSendErrorEvent;
                    event->mPDU = pduHolder->pdu;
                    events.push_back(event);
                }
                mPDUQueue.pop_front();
                mQueueSemaphore.Post();
                mSendErrors++;
            }
        }
    }

    // can't call this with mutex
    foreach(const PDUPeerEventPtr& event, events)
    {
        mEventCallback(event);
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

    if (!isRunning())
    {
        throw EObjectNotRunning();
    }

    if (mEndpoint->RecvPDU(out))
    {
        mTotalReceived++;
        return true;
    }
    else
    {
        return false;
    }
}
