// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerInProcessEndpoint_h_
#define __Forte_PDUPeerInProcessEndpoint_h_

#include "Exception.h"
#include "Object.h"
#include "PDU.h"
#include "PDUPeerEndpoint.h"
#include "AutoMutex.h"
#include "FTrace.h"

namespace Forte
{
    class PDUPeerInProcessEndpoint : public PDUPeerEndpoint
    {
    public:
        PDUPeerInProcessEndpoint();
        virtual ~PDUPeerInProcessEndpoint();

        virtual void CheckConnection() {
            if (!mConnectMessageSent)
            {
                PDUPeerEventPtr event(new PDUPeerEvent());
                event->mEventType = PDUPeerConnectedEvent;
                if (mEventCallback)
                {
                    mEventCallback(event);
                    mConnectMessageSent = true;
                }
            }
        }

        virtual void SendPDU(const Forte::PDU &pdu);
        virtual bool IsPDUReady() const;
        virtual bool RecvPDU(Forte::PDU &out);

        // epoll does not apply here
        virtual void SetEPollFD(int epollFD) {}
        virtual void HandleEPollEvent(const struct epoll_event& e) {}
        virtual void TeardownEPoll() {}

        virtual bool OwnsFD(int fd) const {
            return false;
        }
        virtual bool IsConnected(void) const {
            // we are connected once the callback is setup
            return mEventCallback && mConnectMessageSent;
        }

        void SetFD(int fd) {
            FTRACE2("%d", fd);
            hlog_and_throw(
                HLOG_ERR,
                EUnimplemented(
                    "AddFD called on PDUPeerInProcessEndpoint"));
        }

        virtual PDUPeerStats GetStats() {
            return mStats;
        }

    protected:
        mutable Mutex mMutex;
        std::list<PDUPtr> mPDUBuffer;
        bool mConnectMessageSent;

        PDUPeerStats mStats;
    };

    typedef boost::shared_ptr<PDUPeerInProcessEndpoint> PDUPeerInProcessEndpointPtr;
}
#endif
