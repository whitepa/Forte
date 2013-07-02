#ifndef _GMockPDUPeerEndpoint_h
#define _GMockPDUPeerEndpoint_h

#include "PDUPeerEndpoint.h"
#include <gmock/gmock.h>

namespace Forte
{
    class GMockPDUPeerEndpoint : public PDUPeerEndpoint
    {
    public:
        GMockPDUPeerEndpoint() {}
        ~GMockPDUPeerEndpoint() {}

        MOCK_METHOD0(Start, void());
        MOCK_METHOD0(Shutdown, void());

        MOCK_CONST_METHOD0(IsPDUReady, bool());
        MOCK_CONST_METHOD0(IsConnected, bool());
        MOCK_METHOD1(RecvPDU, bool(Forte::PDU &out));
        MOCK_METHOD1(SetFD, void(int fd));
        MOCK_CONST_METHOD1(OwnsFD, bool(int fd));
        MOCK_METHOD1(HandleEPollEvent, void(const struct epoll_event& e));

    };

    typedef boost::shared_ptr<GMockPDUPeerEndpoint> GMockPDUPeerEndpointPtr;
};
#endif
