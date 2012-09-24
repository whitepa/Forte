#ifndef __PDUPeerSetImpl_h_
#define __PDUPeerSetImpl_h_

#include "AutoFD.h"
#include "AutoMutex.h"
#include "PDUPeerSet.h"
#include "PDUPeer.h"
#include "RWLock.h"
#include <boost/function.hpp>
#include <boost/shared_array.hpp>

namespace Forte
{
    /**
     * PDUPeerSet manages a set of PDUPeer objects, and provides a
     * mechanism for polling the entire set for input simultaneously.
     * 'PDU ready' and 'error' callbacks may be used as part of the
     * polling mechanism.
     */
    class PDUPeerSetImpl : public PDUPeerSet
    {
    public:
        static const int MAX_PEERS = 256;
        static const int RECV_BUFFER_SIZE = 65536;

        PDUPeerSetImpl() : mEPollFD(-1) {}
        virtual ~PDUPeerSetImpl();

        /**
         * GetSize() returns the current number of PDUPeer objects
         * being managed by this PDUPeerSetImpl.
         *
         * @return size of the set
         */
        unsigned int GetSize(void) {
            AutoUnlockMutex peerSetLock(mLock);
            return mPeerSet.size();
        }

        /**
         * PeerCreate will create a new PDUPeer object for the already
         * open peer connection on the given file descriptor.
         *
         * @param fd
         *
         * @return
         */
        boost::shared_ptr<PDUPeer> PeerCreate(int fd);

        /**
         * SendAll will send the given PDU to ALL of the peers being
         * managed by this PDUPeerSetImpl.
         *
         * @param pdu
         */
        void SendAll(const PDU &pdu) const;

        /**
         * SetProcessPDUCallback sets the single callback function to
         * use when a complete PDU has been received on any of the
         * peer connections.
         *
         * @param f
         */
        void SetProcessPDUCallback(CallbackFunc f) {
            mProcessPDUCallback = f;
        }

        /**
         * SetErrorCallback sets the single callback function to use
         * when an unrecoverable error has occurred on any of the peer
         * connections.  The file descriptor within the errored
         * PDUPeer will still be valid at the time the callback is
         * made, but will be closed immediately after the callback
         * returns.
         *
         * @param f
         */
        void SetErrorCallback(CallbackFunc f) {
            mErrorCallback = f;
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
    public:

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
        void PeerDelete(const boost::shared_ptr<Forte::PDUPeer> &peer);

    protected:
        /**
         * mEPollLock protects mEPollFD and mBuffer
         */
        mutable RWLock mEPollLock;
        int mEPollFD;
        boost::shared_array<char> mBuffer;

        /**
         * mLock protects mPeerSet
         */
        mutable Forte::Mutex mLock;
        std::set < boost::shared_ptr<PDUPeer> > mPeerSet;
        CallbackFunc mProcessPDUCallback;
        CallbackFunc mErrorCallback;
    };
};
#endif
