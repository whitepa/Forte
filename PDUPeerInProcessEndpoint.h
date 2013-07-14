// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerInProcessEndpoint_h_
#define __Forte_PDUPeerInProcessEndpoint_h_

#include "Exception.h"
#include "Object.h"
#include "PDU.h"
#include "PDUPeerEndpoint.h"
#include "PDUQueue.h"
#include "AutoMutex.h"
#include "FTrace.h"
#include "Locals.h"
#include "FunctionThread.h"

namespace Forte
{
    class PDUPeerInProcessEndpoint : public PDUPeerEndpoint
    {
    public:
        PDUPeerInProcessEndpoint(const boost::shared_ptr<PDUQueue>& sendQueue);
        virtual ~PDUPeerInProcessEndpoint();

        void Start();
        void Shutdown();

        virtual bool IsPDUReady() const;
        virtual bool RecvPDU(Forte::PDU &out);

        // epoll does not apply here
        virtual void HandleEPollEvent(const struct epoll_event& e) {}

        virtual bool OwnsFD(int fd) const {
            return false;
        }
        virtual bool IsConnected(void) const {
            AutoUnlockMutex lock(mMutex);
            // we are connected once the callback is setup
            return callbackIsValid() && mConnectMessageSent;
        }

        void SetFD(int fd) {
            FTRACE2("%d", fd);
            hlog_and_throw(
                HLOG_ERR,
                EUnimplemented(
                    "AddFD called on PDUPeerInProcessEndpoint"));
        }

    protected:
        void sendThreadRun();
        void connect();

        void callbackThreadRun();
        void triggerCallback(const boost::shared_ptr<PDUPeerEvent>& event);

    protected:
        boost::shared_ptr<Forte::PDUQueue> mSendQueue;

        mutable Mutex mMutex;
        std::list<PDUPtr> mPDUBuffer;
        bool mConnectMessageSent;
        boost::shared_ptr<Forte::FunctionThread> mSendThread;

        boost::shared_ptr<Forte::FunctionThread> mCallbackThread;
        mutable Forte::Mutex mEventQueueMutex;
        Forte::ThreadCondition mEventAvailableCondition;
        std::list<boost::shared_ptr<PDUPeerEvent> > mEventQueue;
    };

    typedef boost::shared_ptr<PDUPeerInProcessEndpoint> PDUPeerInProcessEndpointPtr;
}
#endif
