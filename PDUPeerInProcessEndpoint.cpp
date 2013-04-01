// #SCQAD TAG: forte.pdupeer
#include "PDUPeerInProcessEndpoint.h"
#include "FTrace.h"
#include <boost/make_shared.hpp>

Forte::PDUPeerInProcessEndpoint::PDUPeerInProcessEndpoint()
{
}

Forte::PDUPeerInProcessEndpoint::~PDUPeerInProcessEndpoint()
{
}

void Forte::PDUPeerInProcessEndpoint::SendPDU(const Forte::PDU &pdu)
{
    FTRACE;

    // this is a situation where no one is listening, so sending a PDU
    // should result in an error
    if (!mEventCallback)
    {
        throw EPDUPeerEndpoint("Nothing in process is expecting PDUs");
    }

    {
        AutoUnlockMutex lock(mMutex);
        mPDUBuffer.push_back(PDUPtr(new PDU(pdu)));
    }

    PDUPeerEventPtr event(new PDUPeerEvent());
    event->mEventType = PDUPeerReceivedPDUEvent;
    mEventCallback(event);
}

bool Forte::PDUPeerInProcessEndpoint::IsPDUReady() const
{
    FTRACE;
    AutoUnlockMutex lock(mMutex);
    return (mPDUBuffer.size() > 0);
}

bool Forte::PDUPeerInProcessEndpoint::RecvPDU(Forte::PDU &out)
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
