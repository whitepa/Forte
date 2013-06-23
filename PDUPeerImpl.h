// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerImpl_h_
#define __Forte_PDUPeerImpl_h_

#include "AutoMutex.h"
#include "Event.h"
#include "PDUPeer.h"
#include "PDUPeerTypes.h"
#include "Clock.h"
#include "Semaphore.h"
#include "Locals.h"
#include "CumulativeMovingAverage.h"
#include "FunctionThread.h"

EXCEPTION_SUBCLASS(EPDUPeer, EPDUPeerSendError);
EXCEPTION_SUBCLASS2(EPDUPeer, EPDUPeerNoEventCallback,
                    "Expected PDU Peer event callback function is not set");
EXCEPTION_PARAM_SUBCLASS(EPDUPeer, EPDUPeerQueueFull,
                         "PDU Peer send queue is full (%s)", (unsigned short));
EXCEPTION_PARAM_SUBCLASS(EPDUPeer, EPDUPeerUnknownQueueType,
                         "Unknown queue type: %s", (unsigned short));

namespace Forte
{
    // metadata we need about the pdu that we don't want to send over
    // the wire
    struct PDUHolder {
        Timespec enqueuedTime;
        PDUPtr pdu;
    };
    typedef boost::shared_ptr<PDUHolder> PDUHolderPtr;

    class PDUPeerImpl :
        public PDUPeer,
        public EnableStats<
            PDUPeerImpl,
            Locals<PDUPeerImpl,
            int64_t, int64_t, int64_t,
            int64_t, int64_t, int64_t,
            CumulativeMovingAverage>
            >
    {
    public:
        /**
         * PDUPeerImpl - a PDUPeer is an object that represents a
         * virtual endpoint between to objects. the objects can be
         * connected by various types of endpoints, like a file
         * descriptor, a connecting or listening socket or shared memory
         *
         * @param pduSendTimeout if a node is not connected, how long
         * to wait before giving up
         */
        PDUPeerImpl(
            uint64_t connectingPeerID,
            PDUPeerEndpointPtr endpoint,
            long pduSendTimeout = 2,
            unsigned short queueSize = 1024,
            PDUPeerQueueType queueType = PDU_PEER_QUEUE_THROW);

        virtual ~PDUPeerImpl();

        virtual void Start();
        virtual void Shutdown();

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
        void EnqueuePDU(const PDUPtr &pdu);
        virtual unsigned int GetQueueSize() const {
            AutoUnlockMutex lock(mPDUQueueMutex);
            return mPDUQueue.size();
        }

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
        //TODO: boost::shared_ptr<Forte::PDU> RecvPDU() throws (ENoPDUReady, etc);
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
        void sendThreadRun();
        void sendLoop();
        void lockedEnqueuePDU(const PDUHolderPtr& pdu);
        bool isPDUExpired(PDUHolderPtr pduHolder);
        void failAllPDUs();
        void failExpiredPDUs();

    protected:
        uint64_t mConnectingPeerID;
        PDUPeerEndpointPtr mEndpoint;
        long mPDUSendTimeout;

        // mListMutex will protect the list of PDUs to be sent
        mutable Forte::Mutex mPDUQueueMutex;
        mutable Forte::ThreadCondition mPDUQueueNotEmptyCondition;
        std::deque<PDUHolderPtr> mPDUQueue;
        MonotonicClock mClock;

        // limit size of queue
        unsigned short mQueueMaxSize;
        PDUPeerQueueType mQueueType;
        Semaphore mQueueSemaphore;

        boost::shared_ptr<FunctionThread> mSendThread;

        // stats variables
        int64_t mTotalSent;
        int64_t mTotalReceived;
        int64_t mTotalQueued;
        int64_t mSendErrors;
        int64_t mQueueSize;
        int64_t mStartTime;
        CumulativeMovingAverage mAvgQueueSize;
    };

};
#endif
