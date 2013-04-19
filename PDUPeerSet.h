// #SCQAD TAG: forte.pdupeer
#ifndef __PDU_peer_set_h_
#define __PDU_peer_set_h_

#include "AutoFD.h"
#include "AutoMutex.h"
#include "PDUPeer.h"
#include "RWLock.h"
#include "Types.h"
#include <boost/function.hpp>
#include <boost/shared_array.hpp>
#include "PDUPeerTypes.h"
#include "EnableStats.h"

EXCEPTION_CLASS(EPDUPeerSet);

EXCEPTION_SUBCLASS2(EPDUPeerSet,
                    EPDUPeerDuplicate,
                    "Peer already exists");

EXCEPTION_SUBCLASS2(EPDUPeerSet,
                    EPDUPeerInvalid,
                    "Invalid Peer");

EXCEPTION_SUBCLASS2(EPDUPeerSet,
                    EPDUPeerSetPollFailed,
                    "Failed to poll");

EXCEPTION_SUBCLASS2(EPDUPeerSet,
                    EPDUPeerSetPollCreate,
                    "Failed to create poll socket");

EXCEPTION_SUBCLASS2(EPDUPeerSet,
                    EPDUPeerSetPollAdd,
                    "Failed to add descriptor to poll socket");

EXCEPTION_SUBCLASS2(EPDUPeerSet,
                    EPDUPeerSetNotPolling,
                    "PDUPeerSet is not set up for polling");

EXCEPTION_SUBCLASS2(EPDUPeerSet,
                    EPDUPeerSetNoPeers,
                    "No peers have been added for polling");

namespace Forte
{
    /**
     * PDUPeerSet manages a set of PDUPeer objects, and provides a
     * mechanism for polling the entire set for input simultaneously.
     * 'PDU ready' and 'error' callbacks may be used as part of the
     * polling mechanism.
     */
    class PDUPeerSet :
        public Object,
        public virtual BaseEnableStats
    {
    public:
        PDUPeerSet() {}
        virtual ~PDUPeerSet() {}

        //NOTE: shared_ptr must already exist
        boost::shared_ptr<PDUPeerSet> GetPtr() {
            return boost::static_pointer_cast<PDUPeerSet>(
                Object::shared_from_this());
        }

        /**
         * GetSize() returns the current number of PDUPeer objects
         * being managed by this PDUPeerSet.
         *
         * @return size of the set
         */
        virtual unsigned int GetSize(void) = 0;

        /**
         * returns the number of PDUPeer objects that are currently
         * connected
         *
         * @return count of connected peers
         */
        virtual unsigned int GetConnectedCount(void) = 0;

        /**
         * Thinking of deprecating this. Current use cases for
         * PDUPeerSet use PDUPeers differently than the new
         * design. Will need to figure out if the old way can fit in
         * to the new diesign of if both use cases make sense.
         *
         * PeerCreate will create a new PDUPeer object for the already
         * open peer connection on the given file descriptor.
         *
         * @param fd
         *
         * @return
         */
        virtual boost::shared_ptr<PDUPeer> PeerCreate(int fd) = 0;

        /**
         * PeerEndpointAdd will create an fd to the endpoint object
         * owned by the given PDUPeer
         *
         * @param fd
         *
         * @return
         */
        virtual void PeerAddFD(uint64_t peerID, int fd) = 0;

        /**
         * Broadcast will attempt to send the given PDU to ALL of the
         * peers being managed by this PDUPeerSetImpl. Errors in
         * sending will silently ignored. If an application needs to
         * send to all and handle errors, it should call GetPeer() on
         * each peer it wants to send to handle the error that way
         *
         * @param pdu
         */
        virtual void Broadcast(const PDU &pdu) const = 0;


        /**
         * BroadcastAsync will enqueu the given PDU for each peer
         * being managed by this PDUPeerSetImpl. The error callback
         * will be called according to the the async send rules of the
         * PDUPeer.
         *
         * @param pdu
         */
        virtual void BroadcastAsync(const PDUPtr& pdu) = 0;


        /**
         * SetEventCallback will be called whenever there is an event
         * of interest fired from a PDUPeer. See PDUPeerTypes.h for
         * types types of events and available data
         *
         * @param f
         */
        virtual void SetEventCallback(PDUPeerEventCallback f) = 0;

        /**
         * Creates an epoll file descriptor, and automatically adds
         * all PDUPeer file descriptors to it for polling.  Once
         * created, all subsequent calls to PeerConnected() and
         * fdDisconnected() will add / remove those FDs from the
         * epoll file descriptor, until TeardownEPoll() is called.
         *
         * @return int epoll file descriptor
         */
        virtual int SetupEPoll(void) = 0;

        /**
         * Closes the epoll file descriptor (removing all existing
         * peer descriptors from polling).  If another caller is
         * currently blocked on Poll(), they will receive an
         * exception.
         *
         */
        virtual void TeardownEPoll(void) = 0;

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
        virtual void Poll(int msTimeout = -1, bool interruptible = false) = 0;

        /**
         * PeerDelete will delete the given peer from the PDUPeerSet,
         * and remove the peer from any poll operation in progress.
         *
         * @param peer
         */
        virtual void PeerDelete(const PDUPeerPtr& peer) = 0;

        /**
         * The PDUPeerSetBuilder handles this automatically. If you
         * are setting up a PDUPeerSet yourself, you need to call this
         * if you want events.
         *
         */
        virtual void StartPolling() = 0;

        /**
         * Shutdown all threads, do any other pre-destructor clean up
         */
        virtual void Shutdown() = 0;

        /**
         * Will return the peer as indexed by the peerID. In the case
         * that a peer is created with an FD, this will be the fd it
         * was created with. Otherwise, the peer id passed in by the
         * creator.
         *
         */
        virtual PDUPeerPtr GetPeer(uint64_t peerID) = 0;
    };
    typedef std::pair<uint64_t, PDUPeerPtr> IntPDUPeerPtrPair;
};
#endif
