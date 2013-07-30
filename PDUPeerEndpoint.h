// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerEndpoint_h_
#define __Forte_PDUPeerEndpoint_h_

#include <sys/epoll.h>
#include <sys/types.h>

#include "Exception.h"
#include "LogManager.h"
#include "ThreadedObject.h"
#include "PDU.h"
#include "PDUPeerTypes.h"
#include "EnableStats.h"
#include "Locals.h"
#include <boost/function.hpp>
#include "CumulativeMovingAverage.h"

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
    ECouldNotSendID,
    "Could not send id to peer");

EXCEPTION_SUBCLASS2(
    EPDUPeerEndpoint,
    ENotConnected,
    "Peer has not connected to us");

namespace Forte
{
    class Mutex;
    class Event;

    class PDUPeerEndpoint :
        public ThreadedObject,
        public EnableStats<PDUPeerEndpoint,
                           Locals<PDUPeerEndpoint,
                                  int64_t, int64_t, int64_t,
                                  int64_t, int64_t, int64_t,
                                  int64_t, CumulativeMovingAverage
                                  > >
    {
    public:
        PDUPeerEndpoint()
            : mPDUSendCount(0),
              mPDURecvCount(0),
              mPDUSendErrors(0),
              mByteSendCount(0),
              mByteRecvCount(0),
              mDisconnectCount(0),
              mPDURecvReadyCount(0) {
            registerStatVariable<0>("PDUSendCount",
                                    &PDUPeerEndpoint::mPDUSendCount);

            registerStatVariable<1>("PDURecvCount",
                                    &PDUPeerEndpoint::mPDURecvCount);

            registerStatVariable<2>("PDUSendErrors",
                                    &PDUPeerEndpoint::mPDUSendErrors);

            registerStatVariable<3>("ByteSendCount",
                                    &PDUPeerEndpoint::mByteSendCount);

            registerStatVariable<4>("ByteRecvCount",
                                    &PDUPeerEndpoint::mByteRecvCount);

            registerStatVariable<5>("DisconnectCount",
                                    &PDUPeerEndpoint::mDisconnectCount);

            registerStatVariable<6>("PDURecvReadyCount",
                                    &PDUPeerEndpoint::mPDURecvReadyCount);

            registerStatVariable<7>("PDURecvReadyCountAvg",
                                    &PDUPeerEndpoint::mPDURecvReadyCountAvg);
        }
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
         * Determine if think we are actively connected to our peer
         *
         * @return true if a endpoint is connected
         */
        virtual bool IsConnected(void) const = 0;

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

        /**
         * If an endpoint has registered with epoll, the event will
         * come through here.
         *
         */
        virtual void HandleEPollEvent(const struct epoll_event& e) = 0;

        void SetEventCallback(PDUPeerEventCallback f) {
            AutoUnlockMutex lock(mEventCallbackMutex);
            mEventCallback = f;
        }

    protected:
        void deliverEvent(const boost::shared_ptr<PDUPeerEvent>& event) {
            PDUPeerEventCallback eventCallback;
            {
                AutoUnlockMutex lock(mEventCallbackMutex);
                if (mEventCallback)
                {
                    eventCallback = mEventCallback;
                }
            }

            if (eventCallback)
            {
                eventCallback(event);
            }
        }

        bool callbackIsValid() const {
            AutoUnlockMutex lock(mEventCallbackMutex);
            return (mEventCallback != NULL);
        }

        /**
         * connect if appropriate for this endpoint
         */
        virtual void connect() {}

    protected:
        int mID;

        int64_t mPDUSendCount;
        int64_t mPDURecvCount;
        int64_t mPDUSendErrors;
        int64_t mByteSendCount;
        int64_t mByteRecvCount;
        int64_t mDisconnectCount;
        int64_t mPDURecvReadyCount;
        CumulativeMovingAverage mPDURecvReadyCountAvg;

    private:
        mutable Forte::Mutex mEventCallbackMutex;
        PDUPeerEventCallback mEventCallback;
    };
}

#endif
