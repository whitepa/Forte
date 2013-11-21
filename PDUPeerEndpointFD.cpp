// #SCQAD TAG: forte.pdupeer
#include <sys/epoll.h>
#include <poll.h>
#include <sys/types.h>
#include "AutoMutex.h"
#include "LogManager.h"
#include "PDUPeerEndpointFD.h"
#include "Types.h"
#include "SystemCallUtil.h"
#include "SocketUtil.h"
#include "FileSystemImpl.h"
#include "ProcFileSystem.h"

using namespace Forte;

PDUPeerEndpointFD::PDUPeerEndpointFD(
    const boost::shared_ptr<PDUQueue>& pduSendQueue,
    const boost::shared_ptr<EPollMonitor>& epollMonitor,
    unsigned int sendTimeoutSeconds,
    unsigned int recvBufferSize,
    unsigned int recvBufferMaxSize,
    unsigned int recvBufferStepSize)
    : mPDUSendQueue(pduSendQueue),
      mEPollMonitor(epollMonitor),
      mFD(-1),
      mSendTimeoutSeconds(sendTimeoutSeconds),
      mSendState(SendStateDisconnected),
      mRecvWorkAvailableCondition(mRecvBufferMutex),
      mRecvWorkAvailable(false),
      mRecvBufferSize(recvBufferSize),
      mRecvBufferMaxSize(
          recvBufferMaxSize < recvBufferSize
          ? recvBufferSize
          : recvBufferMaxSize),
      mRecvBufferStepSize(
          recvBufferStepSize < recvBufferSize
          ? recvBufferSize
          : recvBufferStepSize),
      mRecvBuffer(new char[mRecvBufferSize]),
      mCalculator(mRecvBuffer.get(), recvBufferSize),
      mEventAvailableCondition(mEventQueueMutex)
{
    FTRACE2("%d", static_cast<int>(mFD));

    memset(mRecvBuffer.get(), 0, mRecvBufferSize);

    if (!mEPollMonitor)
    {
        hlog_and_throw(HLOG_ERR, Exception("not given valid epoll monitor"));
    }
}

void PDUPeerEndpointFD::Start()
{
    recordStartCall();

    mSendThread.reset(
        new FunctionThread(
            FunctionThread::AutoInit(),
            boost::bind(&PDUPeerEndpointFD::sendThreadRun, this),
            "pdusend-fd"));

    mRecvThread.reset(
        new FunctionThread(
            FunctionThread::AutoInit(),
            boost::bind(&PDUPeerEndpointFD::recvThreadRun, this),
            "pdurecv-fd"));

    mCallbackThread.reset(
        new FunctionThread(
            FunctionThread::AutoInit(),
            boost::bind(&PDUPeerEndpointFD::callbackThreadRun, this),
            "pduclbk-fd"));

}

void PDUPeerEndpointFD::Shutdown()
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

    {
        AutoUnlockMutex lock(mEventQueueMutex);
        mEventAvailableCondition.Signal();
    }

    mSendThread->WaitForShutdown();
    mRecvThread->WaitForShutdown();
    mCallbackThread->WaitForShutdown();
}

void PDUPeerEndpointFD::SetFD(int fd)
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
                    &PDUPeerEndpointFD::HandleEPollEvent,
                    boost::static_pointer_cast<PDUPeerEndpointFD>(
                        shared_from_this()),
                    _1));

            mCalculator.Reset(mRecvBuffer.get(), mRecvBufferSize);
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

void PDUPeerEndpointFD::waitForConnected()
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

void PDUPeerEndpointFD::closeFileDescriptor()
{
    FTRACE;

    bool doCallback = false;
    {
        // stop sending and receiving
        AutoUnlockMutex recvlock(mRecvBufferMutex);
        AutoUnlockMutex fdlock(mFDMutex);
        if (mFD != -1)
        {
            mEPollMonitor->RemoveFD(mFD);
            mFD.Close();
            mFD = -1;
            doCallback = true;
        }
        mPDUSendQueue->Clear();

        mRecvWorkAvailable = true;
        mRecvWorkAvailableCondition.Signal();
        setSendState(SendStateDisconnected);
        mPDUSendQueue->TriggerWaiters();
    }

    if (doCallback)
    {
        PDUPeerEventPtr event(new PDUPeerEvent());
        event->mEventType = PDUPeerDisconnectedEvent;
        triggerCallback(event);
    }
}

void PDUPeerEndpointFD::sendThreadRun()
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

    while (!myThread->IsShuttingDown())
    {
        switch(mSendState)
        {
        case SendStateDisconnected:
            //hlog(HLOG_DEBUG, "mSendState SendStateDisconnected");
            waitForConnected();
            {
                AutoUnlockMutex fdlock(mFDMutex);
                pollFDs[0].fd = mFD;
            }

            setSendState(SendStateConnected);
            break;

        case SendStateConnected:
            //hlog(HLOG_DEBUG, "state SendStateConnected");
            pdu.reset();
            mPDUSendQueue->WaitForNextPDU(pdu);
            if (pdu)
            {
                setSendState(SendStatePDUReady);
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
            setSendState(SendStateBufferAvailable);
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

                    setSendState(SendStateConnected);
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

                    setSendState(SendStateDisconnected);
                    closeFileDescriptor();
                }
            }
            else
            {
                if (errno != ECONNRESET
                    && errno != EBADF)
                {
                    hlog(HLOG_ERR, "unexpected err from send: %d", errno);
                }
                else
                {
                    hlog(HLOG_INFO, "lost connection to peer: %d", errno);
                }
                ++mPDUSendErrors;

                PDUPeerEventPtr event(new PDUPeerEvent());
                event->mEventType = PDUPeerSendErrorEvent;
                event->mPDU = pdu;
                triggerCallback(event);

                setSendState(SendStateDisconnected);
                closeFileDescriptor();
            }
            break;
        }
    }
}

void PDUPeerEndpointFD::setSendState(const SendState& state)
{
    AutoUnlockMutex lock(mSendStateMutex);
    mSendState = state;
}

void PDUPeerEndpointFD::recvThreadRun()
{
    FTRACE;
    hlog(HLOG_DEBUG2, "Starting PDUPeerRecvThread thread");
    Thread* myThread = Thread::MyThread();

    while (!myThread->IsShuttingDown())
    {
        try
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
        catch (std::exception& e)
        {
            hlogstream(HLOG_WARN,
                       "closing connection due to exception: " << e.what());

            closeFileDescriptor();
        }
    }
}

void PDUPeerEndpointFD::recvUntilBlockOrComplete()
{
    const int flags(0);
    int len(1);

    while (len > 0)
    {
        AutoUnlockMutex recvlock(mRecvBufferMutex);
        bufferEnsureHasSpace();

        while ((len = recv(
                    mFD,
                    mCalculator.GetWriteLocation(),
                    mCalculator.GetWriteLength(),
                    flags)) == -1
               && errno == EINTR) {}

        if (len > 0)
        {
            mCalculator.RecordWrite(len);

            // if (mCalculator.Full())
            // {
            //     hlogstream(
            //         HLOG_DEBUG,
            //         "recv buffer became. wrote " << len << " bytes");
            // }

            mByteRecvCount += len;

            if (lockedIsPDUReady())
            {
                PDUPeerEventPtr event(new PDUPeerEvent());
                event->mEventType = PDUPeerReceivedPDUEvent;
                triggerCallback(event);
            }
        }
        updateRecvQueueSizeStats();
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

void PDUPeerEndpointFD::bufferEnsureHasSpace()
{
    try
    {
        while (mCalculator.Full()
               && mRecvBufferSize + mRecvBufferStepSize > mRecvBufferMaxSize)
        {
            mRecvWorkAvailableCondition.Wait();
        }

        if (mCalculator.Full())
        {
            // increase size by step size. we do this creating a new
            // buffer that is this size + step size, copying the old
            // data into the new buffer, and updating the calculator
            // with the new size, new address, and restore the offset
            size_t newsize = mRecvBufferSize + mRecvBufferStepSize;

            if (newsize > mRecvBufferMaxSize)
                hlog_and_throw(HLOG_ERR, EPeerBufferOverflow());

            // new buffer
            boost::shared_array<char> tmp(new char[newsize]);
            memset (tmp.get(), 0, newsize);
            size_t readLengthOldBuffer = mCalculator.GetReadLength();
            // copy old data
            mCalculator.ObjectCopy(tmp.get(), readLengthOldBuffer);
            mRecvBuffer.swap(tmp);
            // update calculator with new addres and size
            mRecvBufferSize = newsize;
            mCalculator.Reset(mRecvBuffer.get(), mRecvBufferSize);
            // update with proper offest. TODO: make this part of Reset
            mCalculator.RecordWrite(readLengthOldBuffer);

            hlogstream(HLOG_DEBUG,
                       "PDU recv buf new size " << mRecvBufferSize
                       << ", " << mCalculator.GetReadLength()
                       << " bytes from old buffer and "
                       << mCalculator.GetWriteLength()
                       << " bytes available for writing");
        }
    }
    catch (std::bad_alloc &e)
    {
        throw EPeerBufferOutOfMemory();
    }
}

bool PDUPeerEndpointFD::lockedIsPDUReady() const
{
    if (mCalculator.GetReadLength() < sizeof(PDUHeader))
    {
        return false;
    }

    if (mCalculator.ObjectWillWrap(sizeof(PDUHeader)))
    {
        mCalculator.ObjectCopy(
            reinterpret_cast<char*>(&mTmpPDUHeader), sizeof(PDUHeader));

        if (mCalculator.GetReadLength() >= PDU::Size(mTmpPDUHeader))
            return true;
        else
            return false;
    }
    else
    {
        const PDUHeader* pduHeader =
            reinterpret_cast<const PDUHeader*>(mCalculator.GetReadLocation());

        if (mCalculator.GetReadLength() >= PDU::Size(*pduHeader))
            return true;
        else
            return false;
    }
}

void PDUPeerEndpointFD::updateRecvQueueSizeStats()
{
    if (mCalculator.Empty())
    {
        mPDURecvReadyCountAvg = mPDURecvReadyCount = 0;
    }
    else
    {
        size_t readyCount(0);
        size_t readLength(0);
        RingBufferCalculator calc(mCalculator);

        while ((readLength = calc.GetReadLength()) >= sizeof(PDUHeader))
        {
            if (calc.ObjectWillWrap(sizeof(PDUHeader)))
            {
                calc.ObjectCopy(
                    reinterpret_cast<char*>(&mTmpPDUHeader), sizeof(PDUHeader));
                calc.RecordRead(std::min(PDU::Size(mTmpPDUHeader), readLength));
            }
            else
            {
                const PDUHeader* pduHeader =
                    reinterpret_cast<const PDUHeader*>(
                        calc.GetReadLocation());
                calc.RecordRead(std::min(PDU::Size(*pduHeader), readLength));
            }
            ++readyCount;
        }

        mPDURecvReadyCountAvg = mPDURecvReadyCount = readyCount;
    }
}

bool PDUPeerEndpointFD::RecvPDU(PDU &out)
{
    {
        AutoUnlockMutex recvlock(mRecvBufferMutex);

        if (!lockedIsPDUReady())
            return false;

        // copy the PDU to out happens in 3 stages
        // copy the header
        // copy the payload
        // copy the optionalData if it exists
        //
        // in order to avoid copying the memory when not needed, we
        // will copy directly from the recv buffer to the pdu. there
        // is a special case where the header, payload or optionalData
        // can wrap from the end to the begining of the buffer.
        //
        // the header is unique because we have to read it to learn
        // about the other two pieces of data. in the case the header
        // wraps, we have to copy it to use it.
        //
        // the payload and optionalData could be copied to the out
        // buffer when they wrap without copying for them, but for
        // that less common case and ease of coding, for now they just
        // get copied

        if (mCalculator.ObjectWillWrap(sizeof(PDUHeader)))
        {
            PDUHeader pduHeader;
            mCalculator.ObjectCopy(
                reinterpret_cast<char*>(&pduHeader), sizeof(PDUHeader));
            out.SetHeader(pduHeader);
            mCalculator.RecordRead(sizeof(PDUHeader));

            if (pduHeader.payloadSize > 0)
            {
                copyPayloadToPDU(out, pduHeader);
            }

            if (pduHeader.optionalDataSize > 0)
            {
                copyOptionalDataToPDU(out, pduHeader);
            }
        }
        else
        {
            const PDUHeader *pduHeader =
                reinterpret_cast<const PDUHeader*>(mCalculator.GetReadLocation());
            out.SetHeader(*pduHeader);
            mCalculator.RecordRead(sizeof(PDUHeader));

            if (pduHeader->payloadSize > 0)
            {
                copyPayloadToPDU(out, *pduHeader);
            }

            if (pduHeader->optionalDataSize > 0)
            {
                copyOptionalDataToPDU(out, *pduHeader);
            }
        }

        ++mPDURecvCount;
        updateRecvQueueSizeStats();

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
    unsigned int basePDUVersion =
        PDU::GetBasePDUVersion(out.GetHeader().version);
    if (basePDUVersion != PDU::PDU_VERSION)
    {
        if (hlog_ratelimit(60))
            hlogstream(HLOG_ERR, "invalid PDU version."
                       << " expected " << PDU::PDU_VERSION
                       << " received " << basePDUVersion);
        //close the fd, this stream will never recover
        closeFileDescriptor();
        throw EPDUVersionInvalid();
    }

    return true;
}

void PDUPeerEndpointFD::copyPayloadToPDU(PDU& out, const PDUHeader& pduHeader)
{
    if (mCalculator.ObjectWillWrap(pduHeader.payloadSize))
    {
        //TODO: copy this data directly into out's buffer
        boost::shared_array<char> tmp(new char[pduHeader.payloadSize]);
        mCalculator.ObjectCopy(tmp.get(), pduHeader.payloadSize);
        out.SetPayload(pduHeader.payloadSize, tmp.get());
        mCalculator.RecordRead(pduHeader.payloadSize);
    }
    else
    {
        out.SetPayload(pduHeader.payloadSize, mCalculator.GetReadLocation());
        mCalculator.RecordRead(pduHeader.payloadSize);
    }
}

void PDUPeerEndpointFD::copyOptionalDataToPDU(
    PDU& out, const PDUHeader& pduHeader)
{
    boost::shared_ptr<PDUOptionalData> od(
        new PDUOptionalData(pduHeader.optionalDataSize,
                            pduHeader.optionalDataAttributes));

    mCalculator.ObjectCopy(
        static_cast<char*>(od->mData), pduHeader.optionalDataSize);
    out.SetOptionalData(od);
    mCalculator.RecordRead(pduHeader.optionalDataSize);
}

void PDUPeerEndpointFD::triggerCallback(
    const boost::shared_ptr<PDUPeerEvent>& event)
{
    AutoUnlockMutex lock(mEventQueueMutex);
    mEventQueue.push_back(event);
    mEventAvailableCondition.Signal();
}

void PDUPeerEndpointFD::callbackThreadRun()
{
    Forte::Thread* thisThread = Forte::Thread::MyThread();
    boost::shared_ptr<Forte::PDUPeerEvent> event;

    while (!thisThread->IsShuttingDown())
    {
        {
            AutoUnlockMutex lock(mEventQueueMutex);
            while (mEventQueue.empty()
                   && !thisThread->IsShuttingDown())
            {
                mEventAvailableCondition.Wait();
            }

            if (thisThread->IsShuttingDown())
            {
                return;
            }

            event = mEventQueue.front();
            mEventQueue.pop_front();
        }

        try
        {
            deliverEvent(event);
        }
        catch (std::exception& e)
        {
            hlogstream(HLOG_ERR, "exception in callback: " << e.what());
        }
        event.reset();
    }
}

bool PDUPeerEndpointFD::IsPDUReady() const
{
    AutoUnlockMutex recvlock(mRecvBufferMutex);
    return lockedIsPDUReady();
}

void PDUPeerEndpointFD::HandleEPollEvent(
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
