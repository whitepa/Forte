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
            unsigned int recvBufferSize = RECV_BUFFER_SIZE,
            unsigned int recvBufferMaxSize = DEFAULT_MAX_BUFFER_SIZE,
            unsigned int recvBufferStepSize = RECV_BUFFER_SIZE);
        virtual ~PDUPeerFileDescriptorEndpoint() {}

        virtual void Start();
        virtual void Shutdown();

        //happens after constructor as a callback will be created and
        //a connect event will be fired
        void SetFD(int FD);
        int GetFD() const {
            AutoUnlockMutex fdlock(mFDMutex);
            return mFD;
        }

        virtual void HandleEPollEvent(const epoll_event& e);
        bool IsPDUReady() const;
        bool RecvPDU(Forte::PDU &out);

        virtual bool IsConnected() const {
            AutoUnlockMutex fdlock(mFDMutex);
            return (mFD != -1);
        }

        virtual bool OwnsFD(int fd) const {
            AutoUnlockMutex fdlock(mFDMutex);
            return (mFD != -1 && mFD == fd);
        }

    private:
        void waitForConnected();
        void closeFileDescriptor();

        enum SendState {
            SendStateDisconnected,
            SendStateConnected,
            SendStatePDUReady,
            SendStateBufferAvailable
        };
        void sendThreadRun();
        void setSendState(const SendState& state);

        void recvThreadRun();
        void recvUntilBlockOrComplete();
        void bufferEnsureHasSpace();
        bool lockedIsPDUReady() const;
        void triggerCallback(const boost::shared_ptr<PDUPeerEvent>& event);
        void updateRecvQueueSizeStats();

        void callbackThreadRun();

    private:
        boost::shared_ptr<PDUQueue> mPDUSendQueue;
        boost::shared_ptr<EPollMonitor> mEPollMonitor;
        boost::shared_ptr<Forte::FunctionThread> mRecvThread;
        boost::shared_ptr<Forte::FunctionThread> mSendThread;
        boost::shared_ptr<Forte::FunctionThread> mCallbackThread;

        // since two threads are waiting on connection, they will both
        // be trying to connect without this lock. could someday be
        // made more elegant
        mutable Forte::Mutex mConnectMutex;
        mutable Forte::Mutex mFDMutex;
        AutoFD mFD;

        const int mSendTimeoutSeconds;
        mutable Forte::Mutex mSendStateMutex;
        SendState mSendState;

        mutable Forte::Mutex mRecvBufferMutex;
        Forte::ThreadCondition mRecvWorkAvailableCondition;
        bool mRecvWorkAvailable;
        size_t mRecvBufferSize;
        const size_t mRecvBufferMaxSize;
        const size_t mRecvBufferStepSize;
        size_t volatile mRecvCursor;
        boost::shared_array<char> mRecvBuffer;

        mutable Forte::Mutex mEventQueueMutex;
        Forte::ThreadCondition mEventAvailableCondition;
        std::list<boost::shared_ptr<PDUPeerEvent> > mEventQueue;
    };
};
#endif
