// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerFileDescriptorEndpoint_h_
#define __Forte_PDUPeerFileDescriptorEndpoint_h_

#include <sys/epoll.h>
#include <sys/types.h>
#include "AutoMutex.h"
#include "AutoFD.h"
#include "PDUQueue.h"
#include <boost/shared_array.hpp>
#include "Locals.h"
#include "EPollMonitor.h"
#include "FunctionThread.h"

namespace Forte
{
    static const int RECV_BUFFER_SIZE = 65536;
    static const int DEFAULT_MAX_BUFFER_SIZE = 1048576;
    static const int DEFAULT_SEND_TIMEOUT = 20*1000;

    class PDUPeerFileDescriptorEndpoint : public PDUPeerEndpoint
    {
    public:
        PDUPeerFileDescriptorEndpoint(
            const boost::shared_ptr<Forte::PDUQueue>& pduSendQueue,
            const boost::shared_ptr<EPollMonitor>& epollMonitor,
            unsigned int sendTimeoutSeconds = DEFAULT_SEND_TIMEOUT,
            unsigned int receiveBufferSize = RECV_BUFFER_SIZE,
            unsigned int receiveBufferMaxSize = DEFAULT_MAX_BUFFER_SIZE,
            unsigned int receiveBufferStepSize = RECV_BUFFER_SIZE);
        virtual ~PDUPeerFileDescriptorEndpoint() {}

        virtual void Start();
        virtual void Shutdown();

        //happens after constructor as a callback will be created and
        //a connect event will be fired
        void SetFD(int FD);
        int GetFD(void) const {
            AutoUnlockMutex fdlock(mFDMutex);
            return mFD;
        }

        virtual void HandleEPollEvent(const epoll_event& e);
        bool IsPDUReady(void) const;
        bool RecvPDU(Forte::PDU &out);

        virtual bool IsConnected(void) const {
            AutoUnlockMutex fdlock(mFDMutex);
            return (mFD != -1);
        }

        virtual bool OwnsFD(int fd) const {
            AutoUnlockMutex fdlock(mFDMutex);
            return (mFD != -1 && mFD == fd);
        }

    protected:
        void waitForConnected();
        bool lockedIsPDUReady(void) const;
        void callbackIfPDUReady();
        void bufferEnsureHasSpace();

        void sendThreadRun();
        void recvThreadRun();
        void callbackThreadRun();

        void recvUntilBlockOrComplete();
        void triggerCallback(const boost::shared_ptr<PDUPeerEvent>& event);

    protected:
        boost::shared_ptr<PDUQueue> mPDUSendQueue;
        boost::shared_ptr<EPollMonitor> mEPollMonitor;

        int mSendTimeoutSeconds;
        mutable Forte::Mutex mSendStateMutex;
        enum SendState {
            SendStateDisconnected,
            SendStateConnected,
            SendStatePDUReady,
            SendStateBufferAvailable
        };
        SendState mSendState;
        size_t mRecvBufferMaxSize;
        size_t mRecvBufferStepSize;

        mutable Forte::Mutex mRecvBufferMutex;
        Forte::ThreadCondition mRecvWorkAvailableCondition;
        bool mRecvWorkAvailable;
        size_t mRecvBufferSize;
        boost::shared_array<char> mRecvBuffer;
        size_t volatile mRecvCursor;
        boost::shared_ptr<Forte::FunctionThread> mRecvThread;

        boost::shared_ptr<Forte::FunctionThread> mSendThread;

        // since two threads are waiting on connection, they will both
        // be trying to connect without this lock. should someday be
        // made more elegant
        mutable Forte::Mutex mConnectMutex;

    private:
        void updateRecvQueueSizeStats();

        void setSendState(const SendState& state);
        void closeFileDescriptor();
        mutable Forte::Mutex mFDMutex;
        AutoFD mFD;

        boost::shared_ptr<Forte::FunctionThread> mCallbackThread;
        mutable Forte::Mutex mEventQueueMutex;
        Forte::ThreadCondition mEventAvailableCondition;
        std::list<boost::shared_ptr<PDUPeerEvent> > mEventQueue;
    };
};
#endif
