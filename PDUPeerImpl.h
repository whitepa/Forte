// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerImpl_h_
#define __Forte_PDUPeerImpl_h_

#include "AutoMutex.h"
#include "Event.h"
#include "PDUPeer.h"
#include "PDUPeerTypes.h"
#include "PDUQueue.h"
#include "Clock.h"
#include "Semaphore.h"
#include "Locals.h"
#include "FunctionThread.h"

EXCEPTION_SUBCLASS(EPDUPeer, EPDUPeerSendError);
EXCEPTION_SUBCLASS(EPDUPeer, EPDUPeerDisconnected);

namespace Forte
{
    class PDUPeerImpl :
        public PDUPeer,
        public EnableStats<PDUPeerImpl, Locals<PDUPeerImpl> >
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
            const boost::shared_ptr<PDUQueue>& pduQueue);

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

        unsigned int GetQueueSize() const {
            return mPDUQueue->GetQueueSize();
        }

        //virtual void SendPDU(const PDU& pdu);
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

        virtual void HandleEPollEvent(const struct epoll_event& e) {
            mEndpoint->HandleEPollEvent(e);
        }

        virtual void PDUPeerEndpointEventCallback(PDUPeerEventPtr event);

    protected:
        uint64_t mConnectingPeerID;
        PDUPeerEndpointPtr mEndpoint;
        boost::shared_ptr<Forte::PDUQueue> mPDUQueue;
    };

};
#endif
