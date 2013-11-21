// #SCQAD TAG: forte.pdupeer
#include "PDUQueue.h"
#include "SystemCallUtil.h"
#include "LogManager.h"
#include "Types.h"
#include "PDUPeerTypes.h"
#include "Thread.h"
#include <boost/make_shared.hpp>

using namespace boost;
using namespace Forte;

PDUQueue::PDUQueue(
    long pduSendTimeout,
    unsigned short queueSize,
    PDUPeerQueueType queueType)
    : mPDUSendTimeout(pduSendTimeout),
      mPDUQueueMutex(),
      mPDUQueueNotEmptyCondition(mPDUQueueMutex),
      mPDUQueueNotFullCondition(mPDUQueueMutex),
      mQueueMaxSize(queueSize),
      mQueueType(queueType),
      mTotalQueued(0),
      mQueueSize(0),
      mDropCount(0),
      mAvgQueueSize()
{
    FTRACE;
    registerStatVariable<0>("totalQueued", &PDUQueue::mTotalQueued);
    registerStatVariable<1>("queueSize", &PDUQueue::mQueueSize);
    registerStatVariable<2>("averageQueueSize", &PDUQueue::mAvgQueueSize);
    registerStatVariable<3>("dropCount", &PDUQueue::mDropCount);
}

PDUQueue::~PDUQueue()
{
    FTRACE;
}

void PDUQueue::EnqueuePDU(const PDUPtr& pdu)
{
    // Potential race condition between semaphore & mutex locks
    AutoUnlockMutex lock(mPDUQueueMutex);

    if (mPDUQueue.size()+1 > mQueueMaxSize)
    {
        if (mQueueType == PDU_PEER_QUEUE_BLOCK)
        {
            while (mPDUQueue.size()+1 > mQueueMaxSize &&
                   !Thread::IsForteThreadAndShuttingDown())
            {
                mPDUQueueNotFullCondition.Wait();
            }
            if (Thread::IsForteThreadAndShuttingDown())
                boost::throw_exception(EThreadShutdown());
        }
        else if (mQueueType == PDU_PEER_QUEUE_DROP)
        {
            ++mDropCount;
            if (hlog_ratelimit(60))
                hlogstream(HLOG_WARN, "Dropping PDUs " << mDropCount << " total");

            return;
        }
        else // CALLBACK or THROW
        {
            hlog_and_throw(HLOG_WARN, EPDUQueueFull(mQueueMaxSize));
        }
    }

    mTotalQueued++;
    mAvgQueueSize = mQueueSize = mPDUQueue.size();

    PDUHolderPtr pduHolder(new PDUHolder);
    pduHolder->enqueuedTime = mClock.GetTime();
    pduHolder->pdu = pdu;
    mPDUQueue.push_back(pduHolder);

    mPDUQueueNotEmptyCondition.Signal();
}

void PDUQueue::GetNextPDU(boost::shared_ptr<PDU>& pdu)
{
    AutoUnlockMutex lock(mPDUQueueMutex);
    if (!mPDUQueue.empty())
    {
        pdu = mPDUQueue.front()->pdu;
        mPDUQueue.pop_front();
        mAvgQueueSize = mQueueSize = mPDUQueue.size();
        mPDUQueueNotFullCondition.Signal();
    }
}

void PDUQueue::WaitForNextPDU(PDUPtr& pdu)
{
    AutoUnlockMutex lock(mPDUQueueMutex);
    while (!Thread::IsForteThreadAndShuttingDown()
           && mPDUQueue.empty())
    {
        mPDUQueueNotEmptyCondition.Wait();
    }

    if (!mPDUQueue.empty())
    {
        pdu = mPDUQueue.front()->pdu;
        mPDUQueue.pop_front();
        mAvgQueueSize = mQueueSize = mPDUQueue.size();
        mPDUQueueNotFullCondition.Signal();
    }
    else if (Thread::IsForteThreadAndShuttingDown())
    {
        mPDUQueueNotFullCondition.Broadcast();
        mPDUQueueNotEmptyCondition.Broadcast();
    }
}

bool PDUQueue::isPDUExpired(PDUHolderPtr pduHolder)
{
    Timespec timeout(mPDUSendTimeout, 0);
    Timespec now;
    now = mClock.GetTime();
    if (pduHolder->enqueuedTime + timeout < now)
    {
        return true;
    }

    return false;
}

void PDUQueue::failExpiredPDUs()
{
    FTRACE;

    //std::vector<PDUPeerEventPtr> events;

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
                //TODO: nothing uses this right now
                /*if (mEventCallback)
                {
                    PDUPeerEventPtr event(new PDUPeerEvent());
                    event->mPeer = GetPtr();
                    event->mEventType = PDUPeerSendErrorEvent;
                    event->mPDU = pduHolder->pdu;
                    events.push_back(event);
                    }*/
                mPDUQueue.pop_front();
                mPDUQueueNotFullCondition.Signal();
            }
        }
    }

    // can't call this with mutex
    //foreach(const PDUPeerEventPtr& event, events)
    //{
    //mEventCallback(event);
    //}
}
