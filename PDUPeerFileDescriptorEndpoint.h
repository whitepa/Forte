#ifndef __Forte_PDUPeerFileDescriptorEndpoint_h_
#define __Forte_PDUPeerFileDescriptorEndpoint_h_

#include "AutoMutex.h"
#include "AutoFD.h"
#include "PDUPeer.h"
#include <boost/shared_array.hpp>

namespace Forte
{
    static const int RECV_BUFFER_SIZE = 65536;

    class PDUPeerFileDescriptorEndpoint : public PDUPeerEndpoint
    {
    public:
        PDUPeerFileDescriptorEndpoint(
            int fd, unsigned int bufsize = RECV_BUFFER_SIZE + PDU::PDU_SIZE) :
            mFD(fd),
            mEPollFD(-1),
            mCursor(0),
            mBufSize(bufsize),
            mPDUBuffer(new char[mBufSize])
            {
                FTRACE2("%d", (int) mFD);
                memset(mPDUBuffer.get(), 0, mBufSize);
            }

        virtual ~PDUPeerFileDescriptorEndpoint() {}

        void SetFD(int FD);
        int GetFD(void) const { return mFD; }
        void DataIn(size_t len, char *buf);
        virtual void SendPDU(const Forte::PDU &pdu);

        void SetEPollFD(int epollFD);

        /**
         * Called when you get an epoll event. will call to protected
         * functions below for specific handling
         */
        virtual void HandleEPollEvent(const struct epoll_event& e);

        virtual void TeardownEPoll();

        /**
         * Determine whether a full PDU has been received from the
         * peer, and is ready for the application to get with a
         * RecvPDU call.
         *
         * @return true if a PDU is ready, false otherwise
         */
        bool IsPDUReady(void) const;

        /**
         * Receive a PDU.  Returns true if a PDU was received, false
         * if no complete PDU is ready.
         *
         * @param out
         *
         * @return true if a PDU was received, false if not.
         */
        bool RecvPDU(Forte::PDU &out);

        void Close(void) { mFD.Close(); }

        virtual bool IsConnected(void) const {
            return (mFD != -1);
        }

        virtual bool OwnsFD(int fd) const {
            return (mFD != -1 && mFD == fd);
        }

    protected:
        virtual void handleFileDescriptorClose(const struct epoll_event& e);
        bool lockedIsPDUReady(void) const;
        void lockedRemoveFDFromEPoll();
        void lockedAddFDToEPoll();

    protected:
        mutable Forte::Mutex mFDLock;
        AutoFD mFD;
        mutable Forte::Mutex mEPollLock;
        int mEPollFD;
        size_t mCursor;
        size_t mBufSize;
        boost::shared_array<char> mPDUBuffer;
    };
};
#endif
