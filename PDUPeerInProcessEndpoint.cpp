// #SCQAD TAG: forte.pdupeer
#include "PDUPeerInProcessEndpoint.h"
#include "FTrace.h"
#include <boost/make_shared.hpp>

using namespace Forte;

PDUPeerInProcessEndpoint::PDUPeerInProcessEndpoint(
    const boost::shared_ptr<PDUQueue>& sendQueue)
    : mSendQueue(sendQueue),
      mConnectMessageSent(false)
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

}

void PDUPeerInProcessEndpoint::Shutdown()
{
    recordShutdownCall();

    mSendThread->Shutdown();
    mSendQueue->TriggerWaiters();
    mSendThread->WaitForShutdown();

    mSendThread.reset();
    mSendQueue.reset();
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
            }

            PDUPeerEventPtr event(new PDUPeerEvent());
            event->mEventType = PDUPeerReceivedPDUEvent;
            deliverEvent(event);
        }
    }
}

bool PDUPeerInProcessEndpoint::IsPDUReady() const
{
    FTRACE;
    AutoUnlockMutex lock(mMutex);
    return (mPDUBuffer.size() > 0);
}

bool PDUPeerInProcessEndpoint::RecvPDU(PDU &out)
{
    FTRACE;
    AutoUnlockMutex lock(mMutex);
    if (mPDUBuffer.size() > 0)
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
        return true;
    }

    return false;
}

void PDUPeerInProcessEndpoint::connect()
{
    if (!mConnectMessageSent)
    {
        PDUPeerEventPtr event(new PDUPeerEvent());
        event->mEventType = PDUPeerConnectedEvent;
        deliverEvent(event);

        AutoUnlockMutex lock(mMutex);
        mConnectMessageSent = true;
    }
}
