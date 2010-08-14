#ifndef __PDU_peer_set_h_
#define __PDU_peer_set_h_

#include "PDUPeer.h"

EXCEPTION_CLASS(EPDUPeerSet);
EXCEPTION_SUBCLASS2(EPDUPeerSet, EPDUPeerDuplicate, "Peer already exists");
EXCEPTION_SUBCLASS2(EPDUPeerSet, EPDUPeerInvalid,   "Invalid Peer");

namespace Forte
{
    class PDUPeerSet : public Object
    {
    public:
        static const int MAX_PEERS = 256;

        PDUPeerSet() {};
        virtual ~PDUPeerSet() {};

        boost::shared_ptr<PDUPeer> PeerGet(int fd);
        void PeerDisconnected(int fd);
        void PeerConnected(int fd);

        void SendAll(PDU &pdu);
//        void UpdatePollFDs(struct pollfd [] &pollFDs, int &nfds);

        // map peer FD to peer object
        typedef std::map<int, boost::shared_ptr<PDUPeer> > FDMap;
        typedef std::pair<int, boost::shared_ptr<PDUPeer> > FDPair;

    protected:
        FDMap mFDMap;
    };
};
#endif
