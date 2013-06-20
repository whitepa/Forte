#ifndef __Forte_GMockPDUPeerSetBuilder_h_
#define __Forte_GMockPDUPeerSetBuilder_h_

#include "PDUPeerSetBuilder.h"

namespace Forte
{
    class GMockPDUPeerSetBuilder : public PDUPeerSetBuilder
    {
    public:
        GMockPDUPeerSetBuilder() {}
        virtual ~GMockPDUPeerSetBuilder() {}

        MOCK_METHOD0(Start, void ());
        MOCK_METHOD0(Shutdown, void ());
        MOCK_METHOD1(SetEventCallback, void(PDUPeerEventCallback f));

        MOCK_CONST_METHOD1(Broadcast, void (const PDU &pdu));
        MOCK_METHOD1(BroadcastAsync, void(PDUPtr pdu));

        MOCK_METHOD1(PeerCreate, PDUPeerPtr (int fd));
        MOCK_METHOD1(GetPeer, PDUPeerPtr(uint64_t peerID));
        MOCK_METHOD1(PeerDelete, void (PDUPeerPtr peer));
        MOCK_CONST_METHOD0(GetSize, int());
        MOCK_METHOD0(GetConnectedCount, unsigned int ());
    };

    typedef boost::shared_ptr<GMockPDUPeerSetBuilder> GMockPDUPeerSetBuilderPtr;
}

#endif
