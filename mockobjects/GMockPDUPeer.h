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

        MOCK_CONST_METHOD0(GetFD, int());
        MOCK_METHOD2(DataIn, void(size_t len, char *buf));
        MOCK_CONST_METHOD1(SendPDU, void(const Forte::PDU &pdu));
        MOCK_CONST_METHOD0(IsPDUReady, bool());
        MOCK_METHOD1(RecvPDU, bool(Forte::PDU &out));
        MOCK_METHOD0(Close, void(void));
    };

    typedef boost::shared_ptr<GMockPDUPeer> GMockPDUPeerPtr;
};
#endif
