// #SCQAD TAG: forte.pdupeer
#ifndef __PDUPeerSetImpl_h_
#define __PDUPeerSetImpl_h_

#include <sys/epoll.h>
#include <sys/types.h>
#include "AutoFD.h"
#include "AutoMutex.h"
#include "PDUPeerSet.h"
#include "PDUPeer.h"
#include "RWLock.h"
#include "FTrace.h"
#include "Locals.h"
#include "FunctionThread.h"
#include "EPollMonitor.h"
#include <boost/shared_ptr.hpp>

namespace Forte
{
    class PDUPeerSetImpl;

    // Stats class which will get connected count
    class ConnectedCount {
    public:

        ConnectedCount()
            : mPeerSet (NULL) {}

        ConnectedCount(PDUPeerSetImpl *peerSet)
            : mPeerSet (peerSet) {}
        ~ConnectedCount() {}

        operator int64_t();

    private:
        PDUPeerSetImpl *mPeerSet;
    };

    /**
     * PDUPeerSet manages a set of PDUPeer objects, and provides a
     * mechanism for polling the entire set for input simultaneously.
     * 'PDU ready' and 'error' events may be used as part of the
     * polling mechanism.
     */
    class PDUPeerSetImpl :
        public PDUPeerSet,
        public EnableStats<PDUPeerSetImpl,
                           Locals<PDUPeerSetImpl, ConnectedCount> >
    {
    public:
        PDUPeerSetImpl(const std::vector<PDUPeerPtr>& peers,
                       const boost::shared_ptr<EPollMonitor>& epollMonitor);
        virtual ~PDUPeerSetImpl();

        /**
         * Setup epoll, start relevant threads
         */
        void Start();

        /**
         * stop relevant threads, teardown epoll
         */
        void Shutdown();

        unsigned int GetSize() {
            AutoUnlockMutex peerSetLock(mPDUPeerLock);
            return mPDUPeers.size();
        }

        virtual unsigned int GetConnectedCount();

        /**
         * PeerCreate will create a new PDUPeer object for the already
         * open peer connection on the given file descriptor.
         *
         * @param fd
         *
         * @return
         */
        PDUPeerPtr PeerCreate(int fd);

        void PeerAdd(const boost::shared_ptr<PDUPeer>& p);

        void PeerAddFD(uint64_t peerID, int fd) {
            FTRACE2("%llu, %d", static_cast<unsigned long long>(peerID), fd);
            AutoUnlockMutex peerSetLock(mPDUPeerLock);
            std::map<uint64_t, PDUPeerPtr>::iterator i;
            i = mPDUPeers.find(peerID);
            if (i == mPDUPeers.end())
            {
                //hlog_and_throw(HLOG_ERR, EPDUPeerInvalid());
                hlogstream(HLOG_ERR, "Tried to add fd " << fd
                           << " to peer " << peerID
                           << " but peer does not exist."
                           << " running in a cluster whose nodes were not"
                           << " properly added!");
            }
            else
            {
                mPDUPeers[peerID]->AddFD(fd);
            }
        }

        //void Broadcast(const PDU& pdu) const;
        void BroadcastAsync(const PDUPtr& pdu);

        void SetEventCallback(PDUPeerEventCallback f);

        /**
         * PeerDelete will delete the given peer from the PDUPeerSetImpl,
         * and remove the peer from any poll operation in progress.
         *
         * @param peer
         */
        void PeerDelete(const PDUPeerPtr &peer);

        PDUPeerPtr GetPeer(uint64_t peerID) {
            return mPDUPeers.at(peerID);
        }

    protected:
        void startPolling();

    protected:
        boost::shared_ptr<EPollMonitor> mEPollMonitor;

        mutable Forte::Mutex mPDUPeerLock;
        std::map<uint64_t, PDUPeerPtr> mPDUPeers;

        PDUPeerEventCallback mEventCallback;

        // stat variable to get connectedCount
        ConnectedCount mConnectedCount;
   };
};
#endif
