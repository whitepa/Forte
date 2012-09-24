#ifndef __PDU_peer_h_
#define __PDU_peer_h_

#include "Exception.h"
#include "Object.h"
#include "PDU.h"

EXCEPTION_CLASS(EPDUPeer);
EXCEPTION_SUBCLASS2(EPDUPeer, EPeerBufferOverflow, "Peer buffer overflowed");
EXCEPTION_SUBCLASS2(EPDUPeer, EPeerSendFailed, "Failed to send PDU to peer");
EXCEPTION_SUBCLASS2(EPDUPeer, EPeerSendInvalid, "Attempt to send invalid PDU");
EXCEPTION_SUBCLASS2(EPDUPeer,
                    EPDUVersionInvalid,
                    "Received PDU with invalid version");
EXCEPTION_SUBCLASS2(EPDUPeer,
                    EPDUOpcodeInvalid,
                    "Received PDU with invalid opcode");

namespace Forte
{
    class Mutex;
    class PDUPeer : public Object
    {
    private:
        PDUPeer(const PDUPeer &other) { throw EUnimplemented(); }

    public:
        PDUPeer() {}
        virtual ~PDUPeer() {}

        /**
         * Get a shared pointer to this PDUPeer.  NOTE: A shared_ptr
         * to this object must already exist.
         *
         * @return shared_ptr
         */
        boost::shared_ptr<PDUPeer> GetPtr(void) {
            return boost::static_pointer_cast<PDUPeer>(
                Object::shared_from_this());
        }

        virtual int GetFD(void) const = 0;
        virtual void DataIn(size_t len, char *buf) = 0;
        virtual void SendPDU(const Forte::PDU &pdu) const = 0;

        /**
         * Determine whether a full PDU has been received from the
         * peer, and is ready for the application to get with a
         * RecvPDU call.
         *
         * @return true if a PDU is ready, false otherwise
         */
        virtual bool IsPDUReady(void) const = 0;

        /**
         * Receive a PDU.  Returns true if a PDU was received, false
         * if no complete PDU is ready.
         *
         * @param out
         *
         * @return true if a PDU was received, false if not.
         */
        virtual bool RecvPDU(Forte::PDU &out) = 0;

        virtual void Close(void) = 0;
    };
};
#endif
