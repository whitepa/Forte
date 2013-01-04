// #SCQAD TAG: forte.pdupeer
#ifndef __PDUPeerSetImpl_h_
#define __PDUPeerSetImpl_h_

#include "AutoFD.h"
#include "AutoMutex.h"
#include "PDUPeerSet.h"
#include "PDUPeer.h"
#include "PDUPollThread.h"
#include "RWLock.h"
#include "FTrace.h"
#include "PDUPeerSetWorkHandler.h"
#include "PDUPollThread.h"
#include <boost/function.hpp>
#include <boost/shared_array.hpp>

namespace Forte
{
    /**
     * PDUPeerSet manages a set of PDUPeer objects, and provides a
     * mechanism for polling the entire set for input simultaneously.
     * 'PDU ready' and 'error' events may be used as part of the
     * polling mechanism.
     */
    class PDUPeerSetImpl : public PDUPeerSet
    {
    public:
        PDUPeerSetImpl(
            DispatcherPtr dispatcher,
            PDUPeerSetWorkHandlerPtr workHandler,
            const std::vector<PDUPeerPtr>& peers);

        virtual ~PDUPeerSetImpl();

        unsigned int GetSize() {
            AutoUnlockMutex peerSetLock(mPDUPeerLock);
            return mPDUPeers.size();
        }

        virtual unsigned int GetConnectedCount(void);

        /**
         * PeerCreate will create a new PDUPeer object for the already
         * open peer connection on the given file descriptor.
         *
         * @param fd
         *
         * @return
         */
        PDUPeerPtr PeerCreate(int fd);

        void PeerAddFD(uint64_t peerID, int fd) {
            FTRACE2("%llu, %d", (unsigned long long) peerID, fd);
            AutoUnlockMutex peerSetLock(mPDUPeerLock);
            std::map<uint64_t, PDUPeerPtr>::iterator i;
            i = mPDUPeers.find(peerID);
            if (i == mPDUPeers.end())
            {
                hlog_and_throw(HLOG_DEBUG2, EPDUPeerInvalid());
            }
            mPDUPeers[peerID]->AddFD(fd);
        }

        void Broadcast(const PDU& pdu) const;
        void BroadcastAsync(const PDUPtr& pdu);

        void SetEventCallback(PDUPeerEventCallback f);

        /**
         * Creates an epoll file descriptor, and automatically adds
         * all PDUPeer file descriptors to it for polling.  Once
         * created, all subsequent calls to PeerConnected() and
         * fdDisconnected() will add / remove those FDs from the
         * epoll file descriptor, until TeardownEPoll() is called.
         *
         * @return int epoll file descriptor
         */
        int SetupEPoll();

        /**
         * Closes the epoll file descriptor (removing all existing
         * peer descriptors from polling).  If another caller is
         * currently blocked on Poll(), they will receive an
         * exception.
         *
         */
        void TeardownEPoll();

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
         * @param interruptible will return if the epoll syscall is
         * interrupted by a signal.
         *
         */
        void Poll(int msTimeout = -1, bool interruptible = false);

        /**
         * PeerDelete will delete the given peer from the PDUPeerSetImpl,
         * and remove the peer from any poll operation in progress.
         *
         * @param peer
         */
        void PeerDelete(const PDUPeerPtr &peer);

        /**
         * The PDUPeerSetBuilder handles this automatically. If you
         * are setting up a PDUPeerSet yourself, you need to call this
         * if you want events.
         *
         */
        void StartPolling();

        PDUPeerPtr GetPeer(uint64_t peerID) {
            return mPDUPeers.at(peerID);
        }

    protected:
        DispatcherPtr mWorkDispatcher;
        PDUPeerSetWorkHandlerPtr mWorkHandler;
        PDUPollThreadPtr mPollThread;

        mutable Forte::Mutex mPDUPeerLock;
        std::map<uint64_t, PDUPeerPtr> mPDUPeers;

        mutable RWLock mEPollLock;
        int mEPollFD;

        PDUPeerEventCallback mEventCallback;
   };
};
#endif
