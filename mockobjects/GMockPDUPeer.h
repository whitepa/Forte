#ifndef _GMockPDUPeer_h
#define _GMockPDUPeer_h

#include "PDUPeer.h"
#include <gmock/gmock.h>

namespace Forte
{
    class GMockPDUPeer : public PDUPeer
    {
    public:
        GMockPDUPeer() {}
        ~GMockPDUPeer() {}

        MOCK_METHOD0(Start, void());
        MOCK_METHOD0(Shutdown, void());

        MOCK_CONST_METHOD0(GetFD, int());
        MOCK_CONST_METHOD1(OwnsFD, bool(int fd));
        MOCK_METHOD1(AddFD, void(int fd));
        MOCK_METHOD1(HandleEPollEvent, void(const struct epoll_event& e));
        MOCK_CONST_METHOD0(GetID, const uint64_t());
        MOCK_METHOD1(SetID, void(int id));
        MOCK_METHOD1(EnqueuePDU, void(const PDUPtr& pdu));
        MOCK_CONST_METHOD0(GetQueueSize, unsigned int());
        MOCK_METHOD0(SendNextPDU, void());
        MOCK_METHOD1(SendPDU, void(const PDU &pdu));
        MOCK_CONST_METHOD0(IsPDUReady, bool());
        MOCK_METHOD1(RecvPDU, bool(PDU &out));
        MOCK_CONST_METHOD0(IsConnected, bool());
    };

    typedef boost::shared_ptr<GMockPDUPeer> GMockPDUPeerPtr;
};
#endif
