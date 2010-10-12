#ifndef __PDU_peer_set_h_
#define __PDU_peer_set_h_

#include "AutoFD.h"
#include "AutoMutex.h"
#include "PDUPeer.h"
#include <boost/function.hpp>
#include <boost/shared_array.hpp>

EXCEPTION_CLASS(EPDUPeerSet);
EXCEPTION_SUBCLASS2(EPDUPeerSet, EPDUPeerDuplicate, "Peer already exists");
EXCEPTION_SUBCLASS2(EPDUPeerSet, EPDUPeerInvalid,   "Invalid Peer");
EXCEPTION_SUBCLASS2(EPDUPeerSet, EPDUPeerSetPollFailed, "Failed to poll");
EXCEPTION_SUBCLASS2(EPDUPeerSet, EPDUPeerSetPollCreate, "Failed to create poll socket");
EXCEPTION_SUBCLASS2(EPDUPeerSet, EPDUPeerSetPollAdd, "Failed to add descriptor to poll socket");
EXCEPTION_SUBCLASS2(EPDUPeerSet, EPDUPeerSetNotPolling, "PDUPeerSet is not set up for polling");
EXCEPTION_SUBCLASS2(EPDUPeerSet, EPDUPeerSetNoPeers, "No peers have been added for polling");

namespace Forte
{
    class PDUPeerSet : public Object
    {
    public:
        typedef boost::function<void(PDUPeer &peer)> PDUCallbackFunc;
        static const int MAX_PEERS = 256;
        static const int RECV_BUFFER_SIZE = 65536;

        PDUPeerSet() : mEPollFD(-1) {}
        virtual ~PDUPeerSet() { if (mEPollFD != -1) close(mEPollFD); }

        boost::shared_ptr<PDUPeer> PeerCreate(int fd);

        void SendAll(PDU &pdu);

        void SetProcessPDUCallback(PDUCallbackFunc f) {
            mProcessPDUCallback = f;
        }

        /** 
         * Creates an epoll file descriptor, and automatically adds
         * all PDUPeer file descriptors to it for polling.  Once
         * created, all subsequent calls to PeerConnected() and
         * fdDisconnected() will add / remove those FDs from the
         * epoll file descriptor, until TeardownEPoll() is called.
         * 
         * @return int epoll file descriptor
         */
        int SetupEPoll(void);

        /** 
         * Closes the epoll file descriptor (removing all existing
         * peer descriptors from polling).  If another caller is
         * currently blocked on Poll(), they will receive an
         * exception.
         * 
         */
        void TeardownEPoll(void);

        /** 
         * Poll will poll all current Peers for input, and process any
         * received input via the ProcessPDUCallback.  The callback
         * will be called for each ready PDU until no more fully
         * buffered PDUs exist for each peer in succession.  If the
         * epoll_wait() call is interrupted, Poll() will return
         * immediately.
         * 
         * @param msTimeout timeout in milliseconds before returning.
         * A value of -1 (the default) will wait indefinitely, while a
         * value of 0 will not wait at all.
         */
        void Poll(int msTimeout = -1);

        void PeerDelete(const boost::shared_ptr<Forte::PDUPeer> &peer);
    private:
        void peerDeleteLocked(const boost::shared_ptr<Forte::PDUPeer> &peer);

    protected:
        int mEPollFD;
        Forte::Mutex mLock;
        std::set < boost::shared_ptr<PDUPeer> > mPeerSet;
        boost::shared_array<char> mBuffer;
        PDUCallbackFunc mProcessPDUCallback;
    };
};
#endif
