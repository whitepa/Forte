// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUQueue_h_
#define __Forte_PDUQueue_h_

/**
 * PDUQueue functions as a blocking queue or a ring buffer,
 * depending. probably could be abstracted further or more properly.
 *
 */

#include "Exception.h"
#include "Object.h"
#include "Dispatcher.h"
#include "PDUPeerEndpoint.h"
#include "PDU.h"
#include "FTrace.h"
#include "EnableStats.h"
#include "Locals.h"
#include <boost/bind.hpp>
#include "ThreadedObject.h"
#include "CumulativeMovingAverage.h"

EXCEPTION_CLASS(EPDUQueue);

EXCEPTION_SUBCLASS2(
    EPDUQueue,
    EPDUQueueNoEventCallback,
    "Expected PDU Peer event callback function is not set");

EXCEPTION_PARAM_SUBCLASS(
    EPDUQueue,
    EPDUQueueFull,
    "PDU Peer send queue is full (%s)", (unsigned short));

EXCEPTION_PARAM_SUBCLASS(
    EPDUQueue,
    EPDUQueueUnknownType,
    "Unknown queue type: %s", (unsigned short));

namespace Forte
{
    class Mutex;
    // local metadata about the pdu
    struct PDUHolder {
        Timespec enqueuedTime;
        PDUPtr pdu;
    };
    typedef boost::shared_ptr<PDUHolder> PDUHolderPtr;

    class PDUQueue :
        public Object,
        public EnableStats<PDUQueue,
                           Locals<PDUQueue,
                                  int64_t,
                                  int64_t,
                                  CumulativeMovingAverage,
                                  int64_t
                                  > >
    {
    public:
        PDUQueue(long pduSendTimeout = 2,
                 unsigned short queueSize = 1024,
                 PDUPeerQueueType queueType = PDU_PEER_QUEUE_THROW);
        virtual ~PDUQueue();

        virtual void EnqueuePDU(const boost::shared_ptr<PDU>& pdu);

        // pdu will be null if not ready
        virtual void GetNextPDU(boost::shared_ptr<PDU>& pdu);

        // will block until ready
        virtual void WaitForNextPDU(boost::shared_ptr<PDU>& pdu);
        //threads will wait on WaitForPDUToSend until there is
        //one. they need to able to be canceled for thread shutdown
        virtual void TriggerWaiters() {
            AutoUnlockMutex lock(mPDUQueueMutex);
            mPDUQueueNotEmptyCondition.Broadcast();
        }

        //virtual void DequeuePDU(PDUPtr& pdu);
        virtual unsigned int GetQueueSize() const {
            AutoUnlockMutex lock(mPDUQueueMutex);
            return mPDUQueue.size();
        }
        PDUPeerQueueType GetQueueType() const {
            return mQueueType;
        }

        void Clear() {
            AutoUnlockMutex lock(mPDUQueueMutex);
            mPDUQueue.clear();
            mPDUQueueNotFullCondition.Broadcast();
            mQueueSize = 0;
            mAvgQueueSize = 0;
        }

    protected:
        void lockedEnqueuePDU(const boost::shared_ptr<PDUHolder>& pdu);
        bool isPDUExpired(PDUHolderPtr pduHolder);
        void failAllPDUs();
        void failExpiredPDUs();

    protected:
        long mPDUSendTimeout;
        mutable Forte::Mutex mPDUQueueMutex;
        Forte::ThreadCondition mPDUQueueNotEmptyCondition;
        Forte::ThreadCondition mPDUQueueNotFullCondition;
        std::deque<PDUHolderPtr> mPDUQueue;
        MonotonicClock mClock;
        // limit size of queue
        unsigned short mQueueMaxSize;
        PDUPeerQueueType mQueueType;

        int64_t mTotalQueued;
        int64_t mQueueSize;
        int64_t mDropCount;
        CumulativeMovingAverage mAvgQueueSize;
    };
};
#endif
