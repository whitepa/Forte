// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerSetBuilderImpl_h_
#define __Forte_PDUPeerSetBuilderImpl_h_

#include <stdint.h>
#include "Types.h"
#include "PDUPeerSet.h"
#include "PDUPeerSetBuilder.h"
#include "PDUPeerSetConnectionHandler.h"
#include "OnDemandDispatcher.h"
#include "ThreadPoolDispatcher.h"
#include "ReceiverThread.h"
#include "PDUPeerEndpointFactory.h"
#include "PDUPeerTypes.h"
#include "Locals.h"

EXCEPTION_CLASS(EPDUPeerSetBuilder);

EXCEPTION_SUBCLASS2(EPDUPeerSetBuilder,
                    EPDUPeerSetBuilderCouldNotCreateID,
                    "Could not create unique id");

namespace Forte
{
    class PDUPeerSetBuilderImpl :
        public PDUPeerSetBuilder,
        public EnableStats<PDUPeerSetBuilderImpl,
            Locals<PDUPeerSetBuilderImpl> >
    {
    public:
        PDUPeerSetBuilderImpl(
            const SocketAddress& listenAddress,
            const SocketAddressVector& peers,
            PDUPeerEventCallback eventCallback = NULL,
            int minThreads = 6,
            int maxThreads = 10,
            bool createInProcessPDUPeer = true,
            long pduPeerSendTimeout = 30,
            unsigned short queueSize = 1024,
            PDUPeerQueueType queueType = PDU_PEER_QUEUE_THROW);

        // old school style
        // Callers using this setup should also call
        // SetEventCallback
        // StartPolling
        PDUPeerSetBuilderImpl();
        ~PDUPeerSetBuilderImpl();

        void Broadcast(const PDU& pdu) const {
            FTRACE;
            mPDUPeerSet->Broadcast(pdu);
        }

        void BroadcastAsync(PDUPtr pdu) {
            FTRACE;
            mPDUPeerSet->BroadcastAsync(pdu);
        }

        int GetSize() const {
            return mPDUPeerSet->GetSize();
        }

        uint64_t GetID() const {
            return mID;
        }

        static uint64_t SocketAddressToID(const SocketAddress& sa);

        PDUPeerPtr GetPeer(uint64_t peerID) {
            return mPDUPeerSet->GetPeer(peerID);
        }

        void SetEventCallback(PDUPeerEventCallback f) {
            mPDUPeerSet->SetEventCallback(f);
        }

        void StartPolling() {
            mPDUPeerSet->StartPolling();
        }

        boost::shared_ptr<PDUPeer> PeerCreate(int fd) {
            return mPDUPeerSet->PeerCreate(fd);
        }

        void PeerDelete(Forte::PDUPeerPtr peer) {
            mPDUPeerSet->PeerDelete(peer);
        }

        unsigned int GetConnectedCount() {
            return mPDUPeerSet->GetConnectedCount();
        }

    protected:
        SocketAddress mListenAddress;
        uint64_t mID;
        SocketAddressVector mPeerSocketAddresses;
        PDUPeerSetPtr mPDUPeerSet;

        boost::shared_ptr<PDUPeerSetConnectionHandler> mConnectionHandler;
        boost::shared_ptr<OnDemandDispatcher> mConnectionDispatcher;
        boost::shared_ptr<ReceiverThread> mReceiverThread;
        //TODO: pass in
        boost::shared_ptr<PDUPeerEndpointFactory> mPDUPeerEndpointFactory;
    };
}

#endif
