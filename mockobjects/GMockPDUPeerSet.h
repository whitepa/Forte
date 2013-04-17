#ifndef _GMockPDUPeerSet_h
#define _GMockPDUPeerSet_h

#include "PDUPeerSet.h"
#include <gmock/gmock.h>

namespace Forte
{
    class GMockPDUPeerSet : public PDUPeerSet
    {
    public:
        GMockPDUPeerSet() {}
        ~GMockPDUPeerSet() {}

        MOCK_CONST_METHOD1(Broadcast, void (const PDU &pdu));
        MOCK_METHOD1(BroadcastAsync, void (const PDUPtr& pdu));
        MOCK_METHOD0(GetSize, unsigned int ());
        MOCK_METHOD1(GetPeer, PDUPeerPtr(uint64_t peerID));
        MOCK_METHOD1(SetEventCallback, void(PDUPeerEventCallback f));
        MOCK_METHOD0(SetupEPoll, int ());
        MOCK_METHOD0(TeardownEPoll, void ());
        MOCK_METHOD2(Poll, void (int msTimeout, bool interruptible));
        MOCK_METHOD1(PeerDelete, void (const PDUPeerPtr& peer));
        MOCK_METHOD1(PeerCreate, PDUPeerPtr (int fd));
        MOCK_METHOD0(GetStats, PDUPeerSetStats());

        MOCK_METHOD0(Shutdown, void ());
    };

    typedef boost::shared_ptr<GMockPDUPeerSet> GMockPDUPeerSetPtr;
};
#endif
