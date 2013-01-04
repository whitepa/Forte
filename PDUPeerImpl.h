// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerImpl_h_
#define __Forte_PDUPeerImpl_h_

#include "AutoMutex.h"
#include "Event.h"
#include "PDUPeer.h"
#include "PDUPeerTypes.h"
#include "Clock.h"

EXCEPTION_CLASS(EPDUSendError);

namespace Forte
{
    // metadata we need about the pdu that we don't want to send over
    // the wire
    struct PDUHolder {
        Timespec enqueuedTime;
        PDUPtr pdu;
    };
    typedef boost::shared_ptr<PDUHolder> PDUHolderPtr;

    class PDUPeerImpl : public PDUPeer
    {
    public:
        PDUPeerImpl(uint64_t connectingPeerID,
                    DispatcherPtr dispatcher,
                    PDUPeerEndpointPtr endpoint,
                    long pduPeerSendTimeout = 30)
            : mConnectingPeerID(connectingPeerID),
              mWorkDispatcher(dispatcher),
              mEndpoint(endpoint),
              mPDUPeerSendTimeout(pduPeerSendTimeout),
              mPDUReadyToSendEventPending(false)
            {
                FTRACE;
                mEndpoint->SetEventCallback(
                    boost::bind(&PDUPeer::PDUPeerEndpointEventCallback,
                                this,
                                _1));
            }

        virtual ~PDUPeerImpl() {}

        const uint64_t GetID() const { return mConnectingPeerID; }

        virtual int GetFD() const {
            return mEndpoint->GetFD();
        }
        virtual bool OwnsFD(int fd) const {
            return mEndpoint->OwnsFD(fd);
        }
        virtual void AddFD(int fd) {
            mEndpoint->SetFD(fd);
        }

        /**
         * Enqueue's a PDU to be sent asynchronously by the end point
         *
         * @return true if a PDU is ready, false otherwise
         */
        void EnqueuePDU(const boost::shared_ptr<Forte::PDU>& pdu);
        virtual void SendNextPDU();

        virtual void SendPDU(const PDU& pdu);

        bool IsConnected() const;
        bool IsPDUReady() const;

        /**
         * Receive a PDU.  Returns true if a PDU was received, false
         * if no complete PDU is ready.
         *
         * @param out
         *
         * @return true if a PDU was received, false if not.
         */
        bool RecvPDU(Forte::PDU &out);

        // we will add ourselves to the epoll fd that is passed in
        virtual void SetEPollFD(int epollFD) {
            mEndpoint->SetEPollFD(epollFD);
        }
        virtual void HandleEPollEvent(const struct epoll_event& e) {
            FTRACE;
            mEndpoint->HandleEPollEvent(e);
        }
        virtual void TeardownEPoll() {
            mEndpoint->TeardownEPoll();
        }

    protected:
        void lockedEnqueuePDU(const PDUHolderPtr& pdu);


    protected:
        uint64_t mConnectingPeerID;
        boost::shared_ptr<Dispatcher> mWorkDispatcher;
        PDUPeerEndpointPtr mEndpoint;

        // only one PDUPeer should be in SendNextPDU currently, since
        // there is only a single connection to send PDUs
        // on. mSendMutex will ensure this is the case
        mutable Forte::Mutex mSendMutex;

        // mListLock will protect the list of PDUs to be sent
        mutable Forte::Mutex mListLock;
        std::list<PDUHolderPtr> mPDUList;
        MonotonicClock mClock;
        long mPDUPeerSendTimeout;

        // there is currently a design issue for which this enables a
        // hokey solution. the design is that enqueueing an event will
        // block when the queue is full, but for the queue to empty,
        // it requires a lock that we have. i think the two solutions
        // that don't rquire a major refactoring (the refactoring is
        // eventually needed) are to look at all the events and not
        // enqueue duplicates, or keep track of a flag that tells us
        // if we are currently sending, and if we are not sending,
        // enqueue the event. going to try out the latter as it is
        // less code and time intensive
        bool mPDUReadyToSendEventPending;
    };

    class PDUReadyToSendEvent : public PDUEvent
    {
    public:
        PDUReadyToSendEvent(const PDUPeerPtr& peer)
            : mPeer(peer)
            {
                FTRACE;
            }

        virtual ~PDUReadyToSendEvent() {
            FTRACE;
        }

        void DoWork() {
            FTRACE;
            mPeer->SendNextPDU();
        }

    protected:
        PDUPeerPtr mPeer;
    };

};
#endif
