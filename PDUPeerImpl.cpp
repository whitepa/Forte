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
    FTRACE;

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
            if (!mEndpoint->IsConnected())
            {
                //hlogstream(HLOG_DEBUG, "endpoint is not connected, quit trying");
                break;
            }

            //hlogstream(HLOG_DEBUG, "endpoint is connected");

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

            //hlogstream(HLOG_DEBUG, "have pdu to send");
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
                hlog(HLOG_DEBUG, "Could not send pdu %s", e.what());

                Timespec timeout(mPDUPeerSendTimeout, 0);
                Timespec now;
                now = mClock.GetTime();
                if (pduHolder->enqueuedTime + timeout < now)
                {
                    hlogstream(HLOG_DEBUG, "list is expired, dropping");

                    AutoUnlockMutex lock(mListLock);
                    while (!mPDUList.empty())
                    {
                        if (mEventCallback)
                        {
                            PDUPeerEventPtr event(new PDUPeerEvent());
                            event->mPeer = GetPtr();
                            event->mEventType = PDUPeerSendErrorEvent;
                            event->mPDU = pduHolder->pdu;
                            mEventCallback(event);
                        }
                        mPDUList.pop_front();
                    }
                    //hlogstream(HLOG_DEBUG, "list is empty");
                    break;
                }
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
