// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerImpl_h_
#define __Forte_PDUPeerImpl_h_

#include "AutoMutex.h"
#include "Event.h"
#include "PDUPeer.h"
#include "PDUPeerTypes.h"
#include "Clock.h"
#include "Semaphore.h"

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

    class PDUPeerImpl;

    // having one thread pool per PDUPeer is not as effecient as other
    // methods, but it will move the ball forward. the current
    // implementation allows several threads to get blocked on a
    // single fd, and also needlessly enqueues several events. the
    // thread will just sleep while there is nothing to do and get
    // signaled when there is something enqueued.
    class PDUPeerSendThread : public Forte::Thread
    {
    public:
        PDUPeerSendThread(
            const boost::shared_ptr<PDUPeerImpl>& pduPeer)
            : mPDUPeer(pduPeer) {
            initialized();
        }

        ~PDUPeerSendThread() {
            deleting();
        }
    protected:
        void* run();
        boost::shared_ptr<PDUPeerImpl> mPDUPeer;
    };

    class PDUPeerImpl : public PDUPeer
    {
        friend class PDUPeerSendThread;
    public:
        /**
         * PDUPeerImpl - a PDUPeer is an object that represents a
         * virtual endpoint between to objects. the objects can be
         * connected by various types of endpoints, like a file
         * descriptor, a connecting or listening socket or shared memory
         *
         * @param pduResendTimeout if a pdu fails, the amount of time
         * it should still retry. 0 means that if it does not send on
         * the first time it will not retry
         */
        PDUPeerImpl(
            uint64_t connectingPeerID,
            PDUPeerEndpointPtr endpoint,
            long pduResendTimeout = 0,
            unsigned short queueSize = 1024,
            PDUPeerQueueType queueType = PDU_PEER_QUEUE_THROW);

        virtual void Begin();

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

        virtual void Shutdown();

        virtual ~PDUPeerImpl() {
            if (!mShutdownCalled)
            {
                hlog(HLOG_ERR, "Shutdown not called before ~PDUPeerImp");
            }
            Shutdown();
        }

    protected:
        void sendLoop();
        void lockedEnqueuePDU(const PDUHolderPtr& pdu);
        bool isPDUExpired(PDUHolderPtr pduHolder);
        void emptyList();

    protected:
        uint64_t mConnectingPeerID;
        PDUPeerEndpointPtr mEndpoint;
        long mPDUResendTimeout;

        // mListMutex will protect the list of PDUs to be sent
        mutable Forte::Mutex mPDUQueueMutex;
        mutable Forte::ThreadCondition mPDUQueueNotEmptyCondition;
        std::deque<PDUHolderPtr> mPDUQueue;
        MonotonicClock mClock;

        // limit size of queue
        unsigned short mQueueMaxSize;
        PDUPeerQueueType mQueueType;
        Semaphore mQueueSemaphore;

        boost::shared_ptr<PDUPeerSendThread> mSendThread;

        bool mShutdownCalled;
    };

};
#endif
