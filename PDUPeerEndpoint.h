// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerEndpoint_h_
#define __Forte_PDUPeerEndpoint_h_

#include "Exception.h"
#include "LogManager.h"
#include "Object.h"
#include "PDU.h"
#include "PDUPeerTypes.h"
#include "EnableStats.h"
#include <boost/function.hpp>
#include <sys/epoll.h>

EXCEPTION_CLASS(EPDUPeerEndpoint);

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    EPeerBufferOverflow,
    "Peer buffer overflowed");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    EPeerBufferOutOfMemory,
    "Cannot allocate memory for peer buffer");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    EPeerSendFailed,
    "Failed to send PDU to peer");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    EPeerSendInvalid,
    "Attempt to send invalid PDU");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    EPDUVersionInvalid,
    "Received PDU with invalid version");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    EPDUOpcodeInvalid,
    "Received PDU with invalid opcode");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    ECouldNotConvertIP,
    "Could not convert IP address");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    ECouldNotCreateSocket,
    "Could not create socket");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    ECouldNotConnect,
    "Could not connect to peer");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    ECouldNotSendID,
    "Could not send id to peer");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    ENotConnected,
    "Peer has not connected to us");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    EPDUPeerSetEPollFD,
    "Unable to setup epoll events on given fd");

namespace Forte
{
    class Mutex;
    class Event;

    class PDUPeerEndpoint : public Object,
        public virtual BaseEnableStats
    {
    public:
        PDUPeerEndpoint() {}
        virtual ~PDUPeerEndpoint() {}

        // as we move towards multiple FDs per peer, this will be
        // AddFD. GetFD will become GetFDs. PDUPeerEndpoint becomes
        // owner of this FD and will be responsible for closing it.
        virtual void SetFD(int fd) = 0;

        // existing PDUPeer consumers are built on access to the
        // underlying fd. in the future there may be way to change
        // this
        virtual int GetFD() const {
            hlog_and_throw(
                HLOG_ERR,
                EUnimplemented(
                    "GetFD called on PDUPeerEndpoint"));
        }

        virtual bool OwnsFD(int fd) const = 0;

        virtual int GetID() const {
            return mID;
        }

        virtual void SetID(int id) {
            mID = id;
        }

        /**
         * Get a shared pointer to this PDUPeerEndpoint.  NOTE: A shared_ptr
         * to this object must already exist.
         *
         * @return shared_ptr
         */
        boost::shared_ptr<PDUPeerEndpoint> GetPtr() {
            return boost::static_pointer_cast<PDUPeerEndpoint>(
                Object::shared_from_this());
        }

        /**
         * SendPDU will send the PDU or throw.
         */
        virtual void SendPDU(const Forte::PDU &pdu) = 0;

        /**
         * Determine if think we are actively connected to our peer
         *
         * @return true if a endpoint is connected
         */
        virtual bool IsConnected(void) const = 0;

        /**
         * Checks connections and connects if appropriate. Currently
         * this only applies to 1 of 4 subclasses so something seems
         * wrong about the method and/or class structure.
         *
         * @return verify we are connected, fixup if needed
         */
        virtual void CheckConnection() {}

        /**
         * Determine whether a full PDU has been received from the
         * PDUPeerEndpoint, and is ready for the application to get
         * with a RecvPDU call.
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

        //TODO: this doesn't seem entirely right since only some of
        //the endpoints are polled with epoll. may need some redesign
        virtual void SetEPollFD(int epollFD) {
            hlog_and_throw(
                HLOG_ERR,
                EUnimplemented(
                    "SetEPollFD called on PDUPeerEndpoint"));
        }
        virtual void HandleEPollEvent(const struct epoll_event& e)  {
            hlog_and_throw(
                HLOG_ERR,
                EUnimplemented(
                    "HandleEPollEvent called on PDUPeerEndpoint"));
        }
        virtual void TeardownEPoll() {
            hlog_and_throw(
                HLOG_ERR,
                EUnimplemented(
                    "TeardownEPoll called on PDUPeerEndpoint"));
        }

        void SetEventCallback(PDUPeerEventCallback f) {
            mEventCallback = f;
        }

    protected:
        int mID;
        PDUPeerEventCallback mEventCallback;
    };
}

#endif
