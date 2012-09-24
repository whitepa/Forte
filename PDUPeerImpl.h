#ifndef __Forte_PDUPeerImpl_h_
#define __Forte_PDUPeerImpl_h_

#include "AutoMutex.h"
#include "AutoFD.h"
#include "PDUPeer.h"
#include <boost/shared_array.hpp>

namespace Forte
{
    class PDUPeerImpl : public PDUPeer
    {
    public:
        PDUPeerImpl(int fd, unsigned int bufsize = 65536 + PDU::PDU_SIZE) :
            mFD(fd),
            mCursor(0),
            mBufSize(bufsize),
            mPDUBuffer(new char[mBufSize])
            {
                memset(mPDUBuffer.get(), 0, mBufSize);
            }
        virtual ~PDUPeerImpl() { };

        int GetFD(void) const { return mFD; }
        void DataIn(size_t len, char *buf);
        void SendPDU(const Forte::PDU &pdu) const;

        /**
         * Determine whether a full PDU has been received from the
         * peer, and is ready for the application to get with a
         * RecvPDU call.
         *
         * @return true if a PDU is ready, false otherwise
         */
        bool IsPDUReady(void) const;
    private:
        bool lockedIsPDUReady(void) const;
    public:

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

    protected:
        mutable Forte::Mutex mLock;
        AutoFD mFD;
        size_t mCursor;
        size_t mBufSize;
        boost::shared_array<char> mPDUBuffer;
    };
};
#endif
