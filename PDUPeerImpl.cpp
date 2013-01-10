// #SCQAD TAG: forte.pdupeer
#include "PDUPeerImpl.h"
#include "LogManager.h"
#include "Types.h"
#include "PDUPeerTypes.h"
#include <boost/make_shared.hpp>

using namespace boost;

void Forte::PDUPeerImpl::SendPDU(const Forte::PDU &pdu)
{
    FTRACE;

    mEndpoint->SendPDU(pdu);
}

void Forte::PDUPeerImpl::EnqueuePDU(const Forte::PDUPtr& pdu)
{
    FTRACE;

    AutoUnlockMutex lock(mListLock);
    PDUHolderPtr pduHolder(new PDUHolder);
    pduHolder->enqueuedTime = mClock.GetTime();
    pduHolder->pdu = pdu;
    mPDUList.push_back(pduHolder);
    if (!mPDUReadyToSendEventPending)
    {
        mWorkDispatcher->Enqueue(make_shared<PDUReadyToSendEvent>(GetPtr()));
        mPDUReadyToSendEventPending = true;
    }
}

// should be called by WorkDispatcher
void Forte::PDUPeerImpl::SendNextPDU()
{
    //FTRACE;

    {
        // assumption is that only WorkDispatcher will call this function
        AutoUnlockMutex lock(mListLock);
        mPDUReadyToSendEventPending = false;
    }


    if (mSendMutex.Trylock() == 0)
    {
        AutoUnlockOnlyMutex lock(mSendMutex);

        //hlogstream(HLOG_DEBUG, "got send lock");

        // every loop will check if mPDUList is empty inside the lock
        while (true)
        {
            PDUHolderPtr pduHolder;
            {
                AutoUnlockMutex lock(mListLock);
                if (mPDUList.empty())
                {
                    break;
                }
                // assumption is that only this function will take
                // things off of mPDUList, so we can leave it on the
                // front of the list, rather the pop/push-on-failure
                pduHolder = mPDUList.front();
            }

            if (!mEndpoint->IsConnected())
            {
                //hlogstream(HLOG_DEBUG, "endpoint is not connected, try later");
                if (isPDUExpired(pduHolder))
                {
                    emptyList();
                }
                break;
            }

            try
            {
                mEndpoint->SendPDU(*(pduHolder->pdu));
                //hlog(HLOG_DEBUG, "PDU sent successfully");

                AutoUnlockMutex lock(mListLock);
                mPDUList.pop_front();
                //hlog(HLOG_DEBUG, "PDU sent and removed from list");
            }
            catch (Exception& e)
            {
                hlog(HLOG_DEBUG2, "Could not send pdu %s", e.what());
                if (isPDUExpired(pduHolder))
                {
                    emptyList();
                }
                break;
            }
        }
    }

    AutoUnlockMutex lock(mListLock);
    if (!mPDUList.empty() && !mPDUReadyToSendEventPending)
    {
        mWorkDispatcher->Enqueue(make_shared<PDUReadyToSendEvent>(GetPtr()));
        mPDUReadyToSendEventPending = true;
    }
}

bool Forte::PDUPeerImpl::isPDUExpired(PDUHolderPtr pduHolder)
{
    FTRACE;

    Timespec timeout(mPDUPeerSendTimeout, 0);
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

    AutoUnlockMutex lock(mListLock);
    hlogstream(HLOG_DEBUG,
               "Error sending PDUs to peer " << mConnectingPeerID);

    PDUHolderPtr pduHolder;

    while (!mPDUList.empty())
    {
        if (mEventCallback)
        {
            pduHolder = mPDUList.front();
            PDUPeerEventPtr event(new PDUPeerEvent());
            event->mPeer = GetPtr();
            event->mEventType = PDUPeerSendErrorEvent;
            event->mPDU = pduHolder->pdu;
            mEventCallback(event);
        }
        mPDUList.pop_front();
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
