// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerSetBuilder_h_
#define __Forte_PDUPeerSetBuilder_h_

#include "PDUPeerTypes.h"

namespace Forte
{
    class PDUPeerSetBuilder : public Forte::Object
    {
    public:
        PDUPeerSetBuilder() {}
        virtual ~PDUPeerSetBuilder() {}

        virtual void Broadcast(const PDU& pdu) const = 0;
        virtual void BroadcastAsync(PDUPtr pdu) = 0;
        virtual int GetSize() const = 0;
        virtual PDUPeerPtr GetPeer(uint64_t peerID) = 0;
        virtual void SetEventCallback(PDUPeerEventCallback f) = 0;
        virtual void StartPolling() = 0;
        virtual PDUPeerPtr PeerCreate(int fd) = 0;
        virtual void PeerDelete(Forte::PDUPeerPtr peer) = 0;
        virtual unsigned int GetConnectedCount() = 0;
    };

    //TODO: rename this PDUPeerSet after PDUPeerSet gets a better name
    typedef boost::shared_ptr<PDUPeerSetBuilder> PDUPeerSetBuilderPtr;
}

#endif
