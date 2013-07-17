// #SCQAD TAG: forte.pdupeer
#include <sys/epoll.h>
#include <poll.h>
#include <sys/types.h>
#include "AutoMutex.h"
#include "LogManager.h"
#include "PDUPeerFileDescriptorEndpoint.h"
#include "Types.h"
#include "SystemCallUtil.h"
#include "SocketUtil.h"
#include "FileSystemImpl.h"
#include "ProcFileSystem.h"

using namespace Forte;

PDUPeerFileDescriptorEndpoint::PDUPeerFileDescriptorEndpoint(
    const boost::shared_ptr<PDUQueue>& pduSendQueue,
    const boost::shared_ptr<EPollMonitor>& epollMonitor,
    unsigned int sendTimeoutSeconds,
    unsigned int receiveBufferSize,
    unsigned int receiveBufferMaxSize,
    unsigned int receiveBufferStepSize)
    : mPDUSendQueue(pduSendQueue),
      mEPollMonitor(epollMonitor),
      mSendTimeoutSeconds(sendTimeoutSeconds),
      mRecvBufferMaxSize(receiveBufferMaxSize),
      mRecvBufferStepSize(receiveBufferStepSize),
      mRecvWorkAvailableCondition(mRecvBufferMutex),
      mRecvWorkAvailable(false),
      mRecvBufferSize(receiveBufferSize),
      mRecvBuffer(new char[mRecvBufferSize]),
      mRecvCursor(0),
      mFD(-1),
      mEventAvailableCondition(mEventQueueMutex)
{
    FTRACE2("%d", static_cast<int>(mFD));

    if (mRecvBufferMaxSize < mRecvBufferSize)
        mRecvBufferMaxSize = mRecvBufferSize;
    if (mRecvBufferStepSize > mRecvBufferSize)
        mRecvBufferStepSize = mRecvBufferSize;

    memset(mRecvBuffer.get(), 0, mRecvBufferSize);

    if (!mEPollMonitor)
    {
        hlog_and_throw(HLOG_ERR, Exception("not given valid epoll monitor"));
    }
}

void PDUPeerFileDescriptorEndpoint::Start()
{
    recordStartCall();

    mSendThread.reset(
        new FunctionThread(
            FunctionThread::AutoInit(),
            boost::bind(&PDUPeerFileDescriptorEndpoint::sendThreadRun, this),
            "pdusend-fd"));

    mRecvThread.reset(
        new FunctionThread(
            FunctionThread::AutoInit(),
            boost::bind(&PDUPeerFileDescriptorEndpoint::recvThreadRun, this),
            "pdurecv-fd"));

    mCallbackThread.reset(
        new FunctionThread(
            FunctionThread::AutoInit(),
            boost::bind(&PDUPeerFileDescriptorEndpoint::callbackThreadRun, this),
            "pduclbk-fd"));

}

void PDUPeerFileDescriptorEndpoint::Shutdown()
{
    recordShutdownCall();

    mSendThread->Shutdown();
    mRecvThread->Shutdown();
    mCallbackThread->Shutdown();
    mPDUSendQueue->TriggerWaiters();

    closeFileDescriptor();

    {
        AutoUnlockMutex recvlock(mRecvBufferMutex);
        mRecvWorkAvailable = true;
        mRecvWorkAvailableCondition.Signal();
    }

    // {
    //     AutoUnlockMutex epolloutlock(mEPollOutReceivedMutex);
    //     mEPollOutReceived = true;
    //     mEPollOutReceivedCondition.Signal();
    // }

    {
        AutoUnlockMutex lock(mEventQueueMutex);
        mEventAvailableCondition.Signal();
    }

    mSendThread->WaitForShutdown();
    mRecvThread->WaitForShutdown();
    mCallbackThread->WaitForShutdown();

    mPDUSendQueue.reset();
    mEPollMonitor.reset();
    mSendThread.reset();
    mRecvThread.reset();
    mCallbackThread.reset();
}

void PDUPeerFileDescriptorEndpoint::SetFD(int fd)
{
    FTRACE2("%d", fd);

    closeFileDescriptor();
    bool sendConnect(false);

    {
        AutoUnlockMutex recvlock(mRecvBufferMutex);
        AutoUnlockMutex fdlock(mFDMutex);

        mFD = fd;

        if (fd != -1)
        {
            setSocketNonBlocking(mFD);

            epoll_event ev = { 0, { 0 } };
            ev.events = EPOLLIN | EPOLLRDHUP;
            ev.data.fd = mFD;
            mEPollMonitor->AddFD(
                fd,
                ev,
                boost::bind(
                    &PDUPeerFileDescriptorEndpoint::HandleEPollEvent,
                    boost::static_pointer_cast<PDUPeerFileDescriptorEndpoint>(
                        shared_from_this()),
                    _1));

            // mEPollOutReceived = true;
            // mEPollOutReceivedCondition.Signal();
            mRecvCursor = 0;
            mRecvWorkAvailable = true;
            mRecvWorkAvailableCondition.Signal();
            sendConnect = true;
        }
        else
        {
            mDisconnectCount++;
        }
    }

    if (sendConnect)
    {
        PDUPeerEventPtr event(new PDUPeerEvent());
        event->mEventType = PDUPeerConnectedEvent;
        triggerCallback(event);
    }
}

void PDUPeerFileDescriptorEndpoint::HandleEPollEvent(
    const epoll_event& e)
{
    if (e.events & EPOLLIN)
    {
        AutoUnlockMutex lock(mRecvBufferMutex);
        mRecvWorkAvailable = true;
        mRecvWorkAvailableCondition.Signal();
    }

    if (e.events & EPOLLERR
        || e.events & EPOLLHUP
        || e.events & EPOLLRDHUP)
    {
        closeFileDescriptor();
    }
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
void PDUPeerFileDescriptorEndpoint::sendThreadRun()
{
    FTRACE;
    hlog(HLOG_DEBUG2, "Starting PDUPeerSendThread thread");
    Thread* myThread = Thread::MyThread();

    boost::shared_ptr<PDU> pdu;
    DeadlineClock sendDeadline;
    MonotonicClock mtc;
    Forte::Timespec remainingTime;

    const int flags(MSG_NOSIGNAL);
    int len(0);
    int cursor(0);
    int sendBufferSize(0);
    boost::shared_array<char> sendBuffer;
    int rc;

    struct pollfd pollFDs[1];
    pollFDs[0].events = POLLOUT | POLLERR;
    const int pollFDCount(1);

    enum SendState {
        SendStateDisconnected,
        SendStateConnected,
        SendStatePDUReady,
        SendStateBufferAvailable
    };
    bool continueTryingToSend(true);
    SendState state(SendStateDisconnected);
    while (!myThread->IsShuttingDown())
    {
        switch(state)
        {
        case SendStateDisconnected:
            //hlog(HLOG_DEBUG, "state SendStateDisconnected");
            waitForConnected();
            {
                AutoUnlockMutex fdlock(mFDMutex);
                pollFDs[0].fd = mFD;
            }

            state = SendStateConnected;
            break;

        case SendStateConnected:
            //hlog(HLOG_DEBUG, "state SendStateConnected");
            pdu.reset();
            mPDUSendQueue->WaitForNextPDU(pdu);
            if (pdu)
            {
                state = SendStatePDUReady;
            }
            break;

        case SendStatePDUReady:
            //hlog(HLOG_DEBUG, "state SendStatePDUReady");
            sendBuffer = PDU::CreateSendBuffer(*pdu);
            sendBufferSize =
                sizeof(PDUHeader)
                + pdu->GetHeader().payloadSize
                + pdu->GetHeader().optionalDataSize;
            cursor = 0;
            state = SendStateBufferAvailable;
            sendDeadline.ExpiresInSeconds(mSendTimeoutSeconds);
            break;

        case SendStateBufferAvailable:
            //hlog(HLOG_DEBUG, "state SendStateBufferAvailable");
            while ((len = send(mFD,
                               sendBuffer.get()+cursor,
                               sendBufferSize-cursor,
                               flags)) == -1
                   && errno == EINTR) {}

            if (len > 0)
            {
                cursor += len;
                mByteSendCount += len;

                if (cursor == sendBufferSize)
                {
                    // pdu is done sending
                    sendBufferSize = cursor = 0;
                    sendBuffer.reset();
                    mPDUSendCount++;

                    state = SendStateConnected;
                    continueTryingToSend = false;
                }
            }
            else if (len == -1 && errno == EAGAIN)
            {
                rc = poll(pollFDs,
                          pollFDCount,
                          sendDeadline.GetRemainingSeconds());
                if (rc < 0)
                {
                    hlog(HLOG_ERR, "poll(): %s",
                         SystemCallUtil::GetErrorDescription(errno).c_str());
                }
                else if (rc > 0)
                {
                    continue;
                }
                else
                {
                    //timeout
                    ++mPDUSendErrors;
                    PDUPeerEventPtr event(new PDUPeerEvent());
                    event->mEventType = PDUPeerSendErrorEvent;
                    event->mPDU = pdu;
                    triggerCallback(event);

                    state = SendStateDisconnected;
                    closeFileDescriptor();
                    continueTryingToSend = false;
                }
            }
            else
            {
                hlog(HLOG_ERR, "unexpected err from recv: %d", errno);
                ++mPDUSendErrors;

                PDUPeerEventPtr event(new PDUPeerEvent());
                event->mEventType = PDUPeerSendErrorEvent;
                event->mPDU = pdu;
                triggerCallback(event);

                state = SendStateDisconnected;
                closeFileDescriptor();
                continueTryingToSend = false;
            }
            break;
        }
    }
}

void PDUPeerFileDescriptorEndpoint::waitForConnected()
{
    AutoUnlockMutex connectLock(mConnectMutex);
    //TODO: figure out a way to do this without sleeping
    //todo:ConnectionManager.Connect(ip, port)
    //todo:ConnectionManager.WaitForConnected(ip, port)
    while (!Thread::MyThread()->IsShuttingDown())
    {
        try
        {
            connect();
        }
        catch (ECouldNotConnect& e)
        {
            mPDUSendQueue->Clear();
        }

        {
            AutoUnlockMutex fdlock(mFDMutex);
            if (mFD != -1)
            {
                return;
            }
        }
        Thread::MyThread()->InterruptibleSleep(Timespec::FromSeconds(1));
    }
}

void PDUPeerFileDescriptorEndpoint::recvThreadRun()
{
    FTRACE;
    hlog(HLOG_DEBUG2, "Starting PDUPeerRecvThread thread");
    Thread* myThread = Thread::MyThread();

    while (!myThread->IsShuttingDown())
    {
        waitForConnected();

        {
            AutoUnlockMutex recvlock(mRecvBufferMutex);
            while (!myThread->IsShuttingDown()
                   && !mRecvWorkAvailable)
            {
                mRecvWorkAvailableCondition.Wait();
            }
            mRecvWorkAvailable = false;
        }

        recvUntilBlockOrComplete();
    }
}

void PDUPeerFileDescriptorEndpoint::recvUntilBlockOrComplete()
{
    const int flags(0);
    int len=1;

    while (len > 0)
    {
        AutoUnlockMutex recvlock(mRecvBufferMutex);
        bufferEnsureHasSpace();

        {
            while ((len = recv(mFD,
                               mRecvBuffer.get() + mRecvCursor,
                               mRecvBufferSize - mRecvCursor,
                               flags)) == -1
                   && errno == EINTR) {}
        }

        if (len > 0)
        {
            mRecvCursor += len;
            mByteRecvCount += len;

            if (lockedIsPDUReady())
            {
                PDUPeerEventPtr event(new PDUPeerEvent());
                event->mEventType = PDUPeerReceivedPDUEvent;
                triggerCallback(event);
                mPDURecvReadyCountAvg =
                    mPDURecvReadyCount = lockedPDURecvQueueSize();
            }
            else
            {
                mPDURecvReadyCountAvg =
                    mPDURecvReadyCount = 0;
            }
        }
    }

    if (len < 0)
    {
        if (errno != EAGAIN)
        {
            if (errno == EBADF
                || errno == ECONNRESET)
            {
                closeFileDescriptor();
            }
            else
            {
                hlog(HLOG_WARN,
                     "recv. treating as connection drop: %d, %s",
                     errno,
                     SystemCallUtil::GetErrorDescription(errno).c_str());
                closeFileDescriptor();
            }
        }
    }
    else if (len == 0)
    {
        hlog(HLOG_DEBUG2, "epoll_event was socket shutdown");
        closeFileDescriptor();
    }
}

void PDUPeerFileDescriptorEndpoint::bufferEnsureHasSpace()
{
    try
    {
        while (mRecvCursor == mRecvBufferSize
               && mRecvBufferSize + mRecvBufferStepSize > mRecvBufferMaxSize)
        {
            mRecvWorkAvailableCondition.Wait();
        }

        while (mRecvCursor == mRecvBufferSize)
        {
            size_t newsize = mRecvBufferSize + mRecvBufferStepSize;

            if (newsize > mRecvBufferMaxSize)
                hlog_and_throw(HLOG_ERR, EPeerBufferOverflow());

            char *tmpstr = new char[newsize];
            memset (tmpstr, 0, newsize);
            if (mRecvBufferSize)
                memcpy(tmpstr, mRecvBuffer.get(), mRecvBufferSize);
            mRecvBuffer.reset(tmpstr);
            mRecvBufferSize = newsize;
            hlog(HLOG_DEBUG, "PDU recv buf new size %zu", mRecvBufferSize);
        }
    }
    catch (std::bad_alloc &e)
    {
        throw EPeerBufferOutOfMemory();
    }
}

bool PDUPeerFileDescriptorEndpoint::IsPDUReady(void) const
{
    AutoUnlockMutex recvlock(mRecvBufferMutex);
    return lockedIsPDUReady();
}

bool PDUPeerFileDescriptorEndpoint::lockedIsPDUReady(void) const
{
    // check for a valid PDU
    // (for now, read cursor is always the start of buffer)
    size_t minPDUSize = sizeof(PDUHeader);
    if (mRecvCursor < minPDUSize)
    {
        hlog(HLOG_DEBUG4,
             "buffer contains less data than minimum PDU %zu vs %zu",
             mRecvCursor, minPDUSize);
        return false;
    }

    PDUHeader *pduHeader =
        reinterpret_cast<PDUHeader*>(mRecvBuffer.get());

    if (mRecvCursor >=
        (minPDUSize + pduHeader->payloadSize + pduHeader->optionalDataSize))
        return true;
    else
        return false;
}

int PDUPeerFileDescriptorEndpoint::lockedPDURecvQueueSize() const
{
    assert(lockedIsPDUReady());

    unsigned int cursor = 0;
    PDUHeader *pduHeader;
    size_t minPDUSize = sizeof(PDUHeader);
    int readyCount(0);
    do
    {
        pduHeader = reinterpret_cast<PDUHeader*>(mRecvBuffer.get() + cursor);
        cursor +=
            minPDUSize + pduHeader->payloadSize + pduHeader->optionalDataSize;
        ++readyCount;
    }
    while (mRecvCursor >= cursor);
    return readyCount;
}

void PDUPeerFileDescriptorEndpoint::closeFileDescriptor()
{
    FTRACE;

    bool doCallback = false;
    {
        // stop sending and receiving
        AutoUnlockMutex recvlock(mRecvBufferMutex);
        //AutoUnlockMutex epolloutlock(mEPollOutReceivedMutex);
        AutoUnlockMutex fdlock(mFDMutex);
        if (mFD != -1)
        {
            mEPollMonitor->RemoveFD(mFD);
            mFD.Close();
            mFD = -1;
            doCallback = true;
        }
        mPDUSendQueue->Clear();
    }

    if (doCallback)
    {
        PDUPeerEventPtr event(new PDUPeerEvent());
        event->mEventType = PDUPeerDisconnectedEvent;
        triggerCallback(event);
    }
}
bool PDUPeerFileDescriptorEndpoint::RecvPDU(PDU &out)
{
    {
        AutoUnlockMutex recvlock(mRecvBufferMutex);

        if (!lockedIsPDUReady())
            return false;

        //TODO: this can be incremented sooner, and might be
        //useful. as it is reported currently, will indicate the
        //number of pdus delivered into the consumer
        mPDURecvCount++;

        PDUHeader *pduHeader =
            reinterpret_cast<PDUHeader*>(mRecvBuffer.get());

        size_t len = sizeof(PDUHeader) + pduHeader->payloadSize;

        hlog(HLOG_DEBUG2, "found valid PDU: len=%zu", len);

        out.SetHeader(*pduHeader);
        out.SetPayload(out.GetPayloadSize(),
                       mRecvBuffer.get()+sizeof(PDUHeader));

        if (pduHeader->optionalDataSize > 0)
        {
            boost::shared_ptr<PDUOptionalData> data(
                new PDUOptionalData(
                    pduHeader->optionalDataSize,
                    pduHeader->optionalDataAttributes));

            memcpy(
                data->mData,
                mRecvBuffer.get() + sizeof(PDUHeader) + pduHeader->payloadSize,
                pduHeader->optionalDataSize);

            out.SetOptionalData(data);
        }

        // \TODO this memmove() based method is inefficient.  Implement a
        // proper ring-buffer.

        // move the rest of the buffer and cursor back by 'len' bytes
        size_t totalBufferConsumed = len + pduHeader->optionalDataSize;
        memmove(mRecvBuffer.get(),
                mRecvBuffer.get() + totalBufferConsumed,
                mRecvBufferSize - totalBufferConsumed);

        memset(mRecvBuffer.get() + mRecvBufferSize - totalBufferConsumed,
               0,
               totalBufferConsumed);

        hlog(HLOG_DEBUG2, "found valid PDU: oldCursor=%zu newCursor=%zu",
             mRecvCursor, mRecvCursor - totalBufferConsumed);
        mRecvCursor -= totalBufferConsumed;

        mRecvWorkAvailable = true;
        mRecvWorkAvailableCondition.Signal();
    }

    // \TODO figure out how to do proper opcode validation.
    //
    // one way to do it would be to ask for a valid opcode range, then
    // verify that the opcode is at least within the accepted
    // parameters. other than that might need to leave it up to the
    // consumers.

    // Validate PDU
    if (out.GetHeader().version != PDU::PDU_VERSION)
    {
        if (hlog_ratelimit(60))
            hlogstream(HLOG_ERR, "invalid PDU version."
                       << " expected " << PDU::PDU_VERSION
                       << " received " << out.GetHeader().version);
        //close the fd, this stream will never recover
        closeFileDescriptor();
        throw EPDUVersionInvalid();
    }

    return true;
}

void PDUPeerFileDescriptorEndpoint::triggerCallback(
    const boost::shared_ptr<PDUPeerEvent>& event)
{
    AutoUnlockMutex lock(mEventQueueMutex);
    mEventQueue.push_back(event);
    mEventAvailableCondition.Signal();
}

void PDUPeerFileDescriptorEndpoint::callbackThreadRun()
{
    Forte::Thread* thisThread = Forte::Thread::MyThread();
    boost::shared_ptr<Forte::PDUPeerEvent> event;

    while (!thisThread->IsShuttingDown())
    {
        {
            AutoUnlockMutex lock(mEventQueueMutex);
            while (mEventQueue.empty()
                   && !Forte::Thread::MyThread()->IsShuttingDown())
            {
                mEventAvailableCondition.Wait();
            }
            if (!mEventQueue.empty())
            {
                event = mEventQueue.front();
                mEventQueue.pop_front();
            }
        }

        if (event)
        {
            try
            {
                deliverEvent(event);
            }
            catch (std::exception& e)
            {
                hlogstream(HLOG_WARN, "event callback threw " << e.what());
            }
            event.reset();
        }
    }
}
