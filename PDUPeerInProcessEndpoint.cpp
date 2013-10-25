// #SCQAD TAG: forte.pdupeer
#include "PDUPeerInProcessEndpoint.h"
#include "FTrace.h"
#include <boost/make_shared.hpp>

using namespace Forte;

PDUPeerInProcessEndpoint::PDUPeerInProcessEndpoint(
    const boost::shared_ptr<PDUQueue>& sendQueue)
    : mSendQueue(sendQueue),
      mConnectMessageSent(false),
      mEventAvailableCondition(mEventQueueMutex)
{
}

PDUPeerInProcessEndpoint::~PDUPeerInProcessEndpoint()
{
}

void PDUPeerInProcessEndpoint::Start()
{
    recordStartCall();

    if (!callbackIsValid())
    {
        hlog_and_throw(HLOG_ERR,
                       EPDUPeerEndpoint("Nothing in process is expecting PDUs"));
    }

    mSendThread.reset(
        new FunctionThread(
            FunctionThread::AutoInit(),
            boost::bind(&PDUPeerInProcessEndpoint::sendThreadRun, this),
            "pdusend-fd"));

    mCallbackThread.reset(
        new FunctionThread(
            FunctionThread::AutoInit(),
            boost::bind(&PDUPeerInProcessEndpoint::callbackThreadRun, this),
            "pduclbk-in"));
}

void PDUPeerInProcessEndpoint::Shutdown()
{
    recordShutdownCall();

    mSendThread->Shutdown();
    mCallbackThread->Shutdown();

    mSendQueue->TriggerWaiters();
    {
        AutoUnlockMutex lock(mEventQueueMutex);
        mEventAvailableCondition.Signal();
    }

    mSendThread->WaitForShutdown();
    mCallbackThread->WaitForShutdown();

    mSendThread.reset();
    mSendQueue.reset();
    mCallbackThread.reset();
}

void PDUPeerInProcessEndpoint::sendThreadRun()
{
    FTRACE;
    hlog(HLOG_DEBUG2, "Starting PDUPeerSendThread thread");

    connect();

    Thread* myThread = Thread::MyThread();
    boost::shared_ptr<PDU> pdu;
    while (!myThread->IsShuttingDown())
    {
        pdu.reset();
        mSendQueue->WaitForNextPDU(pdu);

        if (pdu)
        {
            {
                AutoUnlockMutex lock(mMutex);
                mPDUBuffer.push_back(pdu);
                ++mPDUSendCount;
                mPDURecvReadyCountAvg = mPDURecvReadyCount = mPDUBuffer.size();
            }

            PDUPeerEventPtr event(new PDUPeerEvent());
            event->mEventType = PDUPeerReceivedPDUEvent;
            triggerCallback(event);
        }
    }
}

bool PDUPeerInProcessEndpoint::IsPDUReady() const
{
    AutoUnlockMutex lock(mMutex);
    return (!mPDUBuffer.empty());
}

bool PDUPeerInProcessEndpoint::RecvPDU(PDU &out)
{
    AutoUnlockMutex lock(mMutex);
    if (!mPDUBuffer.empty())
    {
        //NOTE: this code is written this way because the original
        // PDUPeer's RecvPDU (now PDUPeerFileDescriptorEndpoint) is
        // copying the PDU data into the PDU& directly from an
        // incoming ring buffer. A shared_ptr could be returned from
        // this function, but all call sites will need to be changed.
        PDUPtr pdu = mPDUBuffer.front();
        out.SetHeader(pdu->GetHeader());
        out.SetPayload(out.GetPayloadSize(), pdu->GetPayload<void*>());
        out.SetOptionalData(pdu->GetOptionalData());

        mPDUBuffer.pop_front();
        ++mPDURecvCount;
        mPDURecvReadyCountAvg = mPDURecvReadyCount = mPDUBuffer.size();
        return true;
    }

    return false;
}

void PDUPeerInProcessEndpoint::connect()
{
    bool needToDoCallback;
    {
        AutoUnlockMutex lock(mMutex);
        needToDoCallback = !mConnectMessageSent;
    }

    if (needToDoCallback)
    {
        PDUPeerEventPtr event(new PDUPeerEvent());
        event->mEventType = PDUPeerConnectedEvent;
        triggerCallback(event);

        AutoUnlockMutex lock(mMutex);
        mConnectMessageSent = true;
    }
}

void PDUPeerInProcessEndpoint::triggerCallback(
    const boost::shared_ptr<PDUPeerEvent>& event)
{
    AutoUnlockMutex lock(mEventQueueMutex);
    mEventQueue.push_back(event);
    mEventAvailableCondition.Signal();
}

void PDUPeerInProcessEndpoint::callbackThreadRun()
{
    Forte::Thread* thisThread = Forte::Thread::MyThread();
    boost::shared_ptr<Forte::PDUPeerEvent> event;

    while (!thisThread->IsShuttingDown())
    {
        {
            AutoUnlockMutex lock(mEventQueueMutex);
            while (mEventQueue.empty()
                   && !Forte::Thread::MyThread()->IsShuttingDown())
            {
                mEventAvailableCondition.Wait();
            }
            if (!mEventQueue.empty())
            {
                event = mEventQueue.front();
                mEventQueue.pop_front();
            }
        }

        if (event)
        {
            try
            {
                deliverEvent(event);
            }
            catch (std::exception& e)
            {
                hlogstream(HLOG_ERR, "exception in callback: " << e.what());
            }
            event.reset();
        }
    }
}
