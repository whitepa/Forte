#ifndef __PDU_peer_h_
#define __PDU_peer_h_

#include "AutoFD.h"
#include "Exception.h"
#include "Object.h"
#include "PDU.h"
#include <boost/shared_array.hpp>

EXCEPTION_CLASS(EPDUPeer);
EXCEPTION_SUBCLASS2(EPDUPeer, EPeerBufferOverflow, "Peer buffer overflowed");
EXCEPTION_SUBCLASS2(EPDUPeer, EPeerSendFailed, "Failed to send PDU to peer");
EXCEPTION_SUBCLASS2(EPDUPeer, EPeerSendInvalid, "Attempt to send invalid PDU");
EXCEPTION_SUBCLASS2(EPDUPeer, EPDUVersionInvalid, "Received PDU with invalid version");
EXCEPTION_SUBCLASS2(EPDUPeer, EPDUOpcodeInvalid, "Received PDU with invalid opcode");

namespace Forte
{
    class PDUPeer : public Object
    {
    private:
        PDUPeer(const PDUPeer &other) { throw EUnimplemented(); }
    public:
        PDUPeer(int fd, unsigned int bufsize = 65536) :
            mFD(fd),
            mCursor(0),
            mBufSize(bufsize),
            mPDUBuffer(new char[mBufSize])
            {
                memset(mPDUBuffer.get(), 0, mBufSize);
            }
        virtual ~PDUPeer() { };

        boost::shared_ptr<PDUPeer> GetPtr(void) {
            return boost::static_pointer_cast<PDUPeer>(Object::shared_from_this());
        }

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
        bool IsPDUReady(void);

        /** 
         * Receive a PDU.  Returns true if a PDU was received, false if no complete PDU is ready.
         * 
         * @param out 
         * 
         * @return true if a PDU was received, false if not.
         */
        bool RecvPDU(Forte::PDU &out);

        void Close(void) { mFD.Close(); }

    protected:
        AutoFD mFD;
        size_t mCursor;
        size_t mBufSize;
        boost::shared_array<char> mPDUBuffer;
    };
};
#endif
