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

        MOCK_METHOD0(GetSize, unsigned int ());
        MOCK_METHOD1(PeerCreate, boost::shared_ptr<PDUPeer> (int fd));
        MOCK_CONST_METHOD1(SendAll, void (const PDU &pdu));
        MOCK_METHOD1(SetProcessPDUCallback, void (CallbackFunc f));
        MOCK_METHOD1(SetErrorCallback, void (CallbackFunc f));
        MOCK_METHOD0(SetupEPoll, int ());
        MOCK_METHOD0(TeardownEPoll, void ());
        MOCK_METHOD2(Poll, void (int msTimeout, bool interruptible));
        MOCK_METHOD1(PeerDelete,
                     void (const boost::shared_ptr<Forte::PDUPeer> &peer));
    };

    typedef boost::shared_ptr<GMockPDUPeerSet> GMockPDUPeerSetPtr;
};
#endif
