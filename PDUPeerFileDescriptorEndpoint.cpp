// #SCQAD TAG: forte.pdupeer
#include "AutoMutex.h"
#include "LogManager.h"
#include "PDUPeerFileDescriptorEndpoint.h"
#include "Types.h"
#include "SystemCallUtil.h"

void Forte::PDUPeerFileDescriptorEndpoint::SetFD(int fd)
{
    FTRACE2("%d, %d", mEPollFD, static_cast<int>(mFD));
    AutoUnlockMutex fdlock(mFDLock);

    removeFDFromEPoll();
    mFD = fd;
    addFDToEPoll();

    PDUPeerEventPtr event(new PDUPeerEvent());
    event->mEventType = PDUPeerConnectedEvent;
    if (mEventCallback)
        mEventCallback(event);
}

void Forte::PDUPeerFileDescriptorEndpoint::SetEPollFD(int epollFD)
{
    FTRACE2("%d, %d", epollFD, static_cast<int>(mFD));
    AutoUnlockMutex fdlock(mFDLock);

    removeFDFromEPoll();
    mEPollFD = epollFD;
    addFDToEPoll();
}

void Forte::PDUPeerFileDescriptorEndpoint::HandleEPollEvent(
    const struct epoll_event& e)
{
    FTRACE;

    if (e.events & EPOLLIN)
    {
        const int flagsPeek = MSG_PEEK | MSG_DONTWAIT;
        char peekBuffer;
        int len;

        while ((len = recv(mFD, &peekBuffer, 1, flagsPeek)) == -1
               && errno == EINTR) {}

        if (len < 0)
        {
            if (errno == ECONNRESET)
            {
                handleFileDescriptorClose();
            }
            else
            {
                //TODO: handle these errors more specifically
                // for now, HLOG_ERR
                // EBADF
                // ECONNREFUSED
                // EFAULT
                // EINVAL
                // ENOMEM
                // ENOTCONN
                // ENOTSOCK
                hlog(HLOG_ERR, "recv failed: %s",
                     SystemCallUtil::GetErrorDescription(errno).c_str());
            }
        }
        else if (len == 0)
        {
            hlog(HLOG_DEBUG2, "epoll_event was socket shutdown");
            handleFileDescriptorClose();
        }
        else
        {
            const int flags(MSG_DONTWAIT);
            while (len > 0)
            {
                {
                    AutoUnlockMutex lock(mReceiveMutex);
                    bufferEnsureHasSpace();

                    while ((len = recv(mFD,
                                       mPDUBuffer.get() + mCursor,
                                       mBufSize - mCursor,
                                       flags)) == -1
                           && errno == EINTR) {}

                    if (len < 0)
                    {
                        if (errno != EWOULDBLOCK)
                            hlog(HLOG_ERR,
                                 "recv failed: %s",
                                 SystemCallUtil
                                 ::GetErrorDescription(errno).c_str());
                    }
                    else
                    {
                        mCursor += len;
                    }
                }
                callbackIfPDUReady();
            }
        }
    }
    else if (
        e.events & EPOLLERR
        || e.events & EPOLLHUP
        || e.events & EPOLLRDHUP)
    {
        handleFileDescriptorClose();
    }
}

void Forte::PDUPeerFileDescriptorEndpoint::bufferEnsureHasSpace()
{
    FTRACE;
    try
    {
        while (mCursor == mBufSize
               && mBufSize + mBufStepSize > mBufMaxSize)
        {
            mBufferFullCondition.Wait();
        }

        while (mCursor == mBufSize)
        {
            size_t newsize = mBufSize + mBufStepSize;

            if (newsize > mBufMaxSize)
                hlog_and_throw(HLOG_ERR, EPeerBufferOverflow());

            char *tmpstr = new char[newsize];
            memset (tmpstr, 0, newsize);
            if (mBufSize)
                memcpy(tmpstr, mPDUBuffer.get(), mBufSize);
            mPDUBuffer.reset(tmpstr);
            mBufSize = newsize;
            hlog(HLOG_DEBUG, "PDU new size %zu", mBufSize);
        }
    }
    catch (std::bad_alloc &e)
    {
        throw EPeerBufferOutOfMemory();
    }
}

void Forte::PDUPeerFileDescriptorEndpoint::callbackIfPDUReady()
{
    if (mEventCallback && IsPDUReady())
    {
        PDUPeerEventPtr event(new PDUPeerEvent());
        event->mEventType = PDUPeerReceivedPDUEvent;
        mEventCallback(event);
    }
}

void Forte::PDUPeerFileDescriptorEndpoint::handleFileDescriptorClose()
{
    FTRACE;
    {
        // stop sending and receiving
        AutoUnlockMutex fdlock(mFDLock);
        AutoUnlockMutex sendlock(mSendMutex);
        AutoUnlockMutex recvlock(mReceiveMutex);
        removeFDFromEPoll();
        if (mFD != -1)
        {
            close(mFD);
            mFD = -1;
        }
    }

    PDUPeerEventPtr event(new PDUPeerEvent());
    event->mEventType = PDUPeerDisconnectedEvent;
    if (mEventCallback)
        mEventCallback(event);
}

void Forte::PDUPeerFileDescriptorEndpoint::TeardownEPoll()
{
    FTRACE;
    AutoUnlockMutex lock(mFDLock);
    removeFDFromEPoll();
    mEPollFD = -1;
}

void Forte::PDUPeerFileDescriptorEndpoint::addFDToEPoll()
{
    FTRACE;

    if (mFD != -1 && mEPollFD != -1)
    {
        hlog(HLOG_DEBUG2,
             "add fd %d to epoll fd %d", static_cast<int>(mFD), mEPollFD);
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLERR | EPOLLRDHUP;
        // NOTE: ptr and fd are in the same union, so reversing this
        // order will break things. assigning ptr to null to get
        // around a valgrind issue
        ev.data.ptr = NULL;
        ev.data.fd = mFD;
        if (epoll_ctl(mEPollFD, EPOLL_CTL_ADD, mFD, &ev) < 0)
        {
            throw EPDUPeerSetEPollFD(
                FString(FStringFC(),
                        "epoll : %d, fd: %d, %s",
                        static_cast<int>(mEPollFD),
                        static_cast<int>(mFD),
                        SystemCallUtil::GetErrorDescription(errno).c_str()));
        }
    }
}

void Forte::PDUPeerFileDescriptorEndpoint::removeFDFromEPoll()
{
    FTRACE;

    if (mEPollFD != -1 && mFD != -1)
    {
        if (epoll_ctl(mEPollFD, EPOLL_CTL_DEL, mFD, NULL) < 0)
        {
            hlog(HLOG_ERR, "EPOLL_CTL_DEL failed: %s",
                 SystemCallUtil::GetErrorDescription(errno).c_str());
        }
    }
}

void Forte::PDUPeerFileDescriptorEndpoint::SendPDU(const Forte::PDU &pdu)
{
    FTRACESTREAM(pdu);

    try
    {
        AutoUnlockMutex lock(mSendMutex);
        size_t offset = 0;
        size_t len =
            sizeof(Forte::PDUHeader)
            + pdu.GetPayloadSize();

        hlog(HLOG_DEBUG2, "sending %zu bytes on fd %d, payloadsize = %u",
             len, mFD.GetFD(), pdu.GetPayloadSize());

        // Put PDU header payload contents into single buffer for sending
        boost::shared_array<char> sendBuf = PDU::CreateSendBuffer(pdu);

        int flags = MSG_NOSIGNAL;
        while (len > 0)
        {
            int sent = send(mFD, sendBuf.get()+offset, len, flags);
            if (sent < 0 && errno != EINTR)
                hlog_and_throw(
                    HLOG_DEBUG,
                    EPeerSendFailed(
                        FStringFC(),
                        "%s",
                        SystemCallUtil::GetErrorDescription(errno).c_str()));
            offset += sent;
            len -= sent;
            mBytesSent += sent;
        }

        if (pdu.GetOptionalDataSize() > 0)
        {
            len = pdu.GetOptionalDataSize();
            offset = 0;
            const char*
                optionalData =
                reinterpret_cast<const char*>(pdu.GetOptionalData()->GetData());

            hlog(HLOG_DEBUG2,
                 "sending %zu bytes of optional data on fd %d",
                 len, mFD.GetFD());

            while (len > 0)
            {
                int sent = send(mFD, optionalData+offset, len, flags);
                if (sent < 0)
                {
                    hlog_and_throw(
                        HLOG_DEBUG,
                        EPeerSendFailed(
                            FStringFC(),
                            "%s",
                            SystemCallUtil::GetErrorDescription(errno).c_str()));
                }
                offset += sent;
                len -= sent;
                mBytesSent += sent;
            }
        }
    }
    catch (Exception& e)
    {
        // it looks like the only error we could actually catch here
        // and potentially recover from is ENOBUFS. rather than add a
        // special case for that situation, going to handle the other
        // 90% and close the connection in this case.
        hlogstream(HLOG_DEBUG, "caught exception sending: " << e.what());
        handleFileDescriptorClose();
        throw;
    }
}

bool Forte::PDUPeerFileDescriptorEndpoint::IsPDUReady(void) const
{
    AutoUnlockMutex lock(mReceiveMutex);
    return lockedIsPDUReady();
}

bool Forte::PDUPeerFileDescriptorEndpoint::lockedIsPDUReady(void) const
{
    // check for a valid PDU
    // (for now, read cursor is always the start of buffer)
    size_t minPDUSize = sizeof(Forte::PDUHeader);
    if (mCursor < minPDUSize)
    {
        hlog(HLOG_DEBUG4,
             "buffer contains less data than minimum PDU %zu vs %zu",
             mCursor, minPDUSize);
        return false;
    }

    Forte::PDUHeader *pduHeader =
        reinterpret_cast<Forte::PDUHeader*>(mPDUBuffer.get());

    // Validate PDU
    if (pduHeader->version != Forte::PDU::PDU_VERSION)
    {
        //TODO: this should probably close the fd, this stream will
        //never recover
        if (hlog_ratelimit(60))
            hlogstream(HLOG_ERR, "invalid PDU version."
                       << " expected " << Forte::PDU::PDU_VERSION
                       << " received " << pduHeader->version);
        throw EPDUVersionInvalid();
    }

    // \TODO figure out how to do proper opcode validation.
    //
    // one way to do it would be to ask for a valid opcode range, then
    // verify that the opcode is at least within the accepted
    // parameters. other than that might need to leave it up to the
    // consumers.

    if (mCursor >=
        (minPDUSize + pduHeader->payloadSize + pduHeader->optionalDataSize))
        return true;
    else
        return false;
}

bool Forte::PDUPeerFileDescriptorEndpoint::RecvPDU(Forte::PDU &out)
{
    AutoUnlockMutex lock(mReceiveMutex);
    if (!lockedIsPDUReady())
        return false;

    Forte::PDUHeader *pduHeader =
        reinterpret_cast<Forte::PDUHeader*>(mPDUBuffer.get());

    size_t len = sizeof(Forte::PDUHeader) + pduHeader->payloadSize;

    hlog(HLOG_DEBUG2, "found valid PDU: len=%zu", len);

    out.SetHeader(*pduHeader);
    out.SetPayload(out.GetPayloadSize(),
                   mPDUBuffer.get()+sizeof(Forte::PDUHeader));

    if (pduHeader->optionalDataSize > 0)
    {
        boost::shared_ptr<Forte::PDUOptionalData> data(
            new Forte::PDUOptionalData(
                pduHeader->optionalDataSize,
                pduHeader->optionalDataAttributes));

        memcpy(
            data->mData,
            mPDUBuffer.get() + sizeof(Forte::PDUHeader) + pduHeader->payloadSize,
            pduHeader->optionalDataSize);

        out.SetOptionalData(data);
    }

    // \TODO this memmove() based method is inefficient.  Implement a
    // proper ring-buffer.

    // move the rest of the buffer and cursor back by 'len' bytes
    size_t totalBufferConsumed = len + pduHeader->optionalDataSize;
    memmove(mPDUBuffer.get(),
            mPDUBuffer.get() + totalBufferConsumed,
            mBufSize - totalBufferConsumed);

    memset(mPDUBuffer.get() + mBufSize - totalBufferConsumed,
           0,
           totalBufferConsumed);

    hlog(HLOG_DEBUG2, "found valid PDU: oldCursor=%zu newCursor=%zu",
         mCursor, mCursor - totalBufferConsumed);
    mCursor -= totalBufferConsumed;

    mBufferFullCondition.Signal();
    return true;
}
