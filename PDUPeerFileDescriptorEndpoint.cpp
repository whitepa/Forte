// #SCQAD TAG: forte.pdupeer
#include "AutoMutex.h"
#include "LogManager.h"
#include "PDUPeerFileDescriptorEndpoint.h"
#include "Types.h"
#include "SystemCallUtil.h"

void Forte::PDUPeerFileDescriptorEndpoint::SetFD(int fd)
{
    FTRACE2("%d, %d", mEPollFD, (int) mFD);
    AutoUnlockMutex fdlock(mFDLock);
    AutoUnlockMutex epolllock(mEPollLock);

    lockedRemoveFDFromEPoll();
    mFD = fd;
    lockedAddFDToEPoll();

    PDUPeerEventPtr event(new PDUPeerEvent());
    event->mEventType = PDUPeerConnectedEvent;
    if (mEventCallback)
        mEventCallback(event);
}

void Forte::PDUPeerFileDescriptorEndpoint::SetEPollFD(int epollFD)
{
    FTRACE2("%d, %d", epollFD, (int) mFD);
    AutoUnlockMutex fdlock(mFDLock);
    AutoUnlockMutex epolllock(mEPollLock);

    lockedRemoveFDFromEPoll();
    mEPollFD = epollFD;
    lockedAddFDToEPoll();
}

void Forte::PDUPeerFileDescriptorEndpoint::HandleEPollEvent(
    const struct epoll_event& e)
{
    FTRACE;

    if (e.events & EPOLLIN)
    {
        char recvBuffer[2048];
        size_t bufferSize = 2048;

        {
            int flags = MSG_PEEK | MSG_DONTWAIT;
            int len;

            while (
                (len = recv(mFD, recvBuffer, bufferSize, flags)) == -1
                && errno == EINTR)
            {
            }

            if (len < 0)
            {
                //TODO: handle these errors more specifically
                // for now, HLOG_ERR
                // EAGAIN or EWOULDBLOCK
                // EBADF
                // ECONNREFUSED
                // EFAULT
                // EINVAL
                // ENOMEM
                // ENOTCONN
                // ENOTSOCK
                hlog(HLOG_ERR, "recv failed: %s",
                     SystemCallUtil::GetErrorDescription(errno).c_str());
                //handleFileDescriptorClose(e);
            }
            else if (len == 0)
            {
                hlog(HLOG_DEBUG2, "epoll_event was socket shutdown");
                // sub classes should override for specialization of FD loss
                handleFileDescriptorClose(e);
            }
            else
            {
                while (len > 0)
                {
                    {
                        AutoUnlockMutex lock(mReceiveMutex);
                        flags = MSG_DONTWAIT;

                        while (
                            (len = recv(mFD, recvBuffer, bufferSize, 0)) == -1
                            && errno == EINTR)
                        {
                        }

                        hlog(HLOG_DEBUG2,
                             "received %d bytes on fd %d", len, mFD.Fd());
                        lockedDataIn(len, recvBuffer);

                        flags = MSG_PEEK | MSG_DONTWAIT;
                        while (
                            (len = recv(mFD, recvBuffer, bufferSize, flags)) == -1
                            && errno == EINTR)
                        {
                        }

                    }
                    callbackIfPDUReady();
                }
            }
        }
    }
    else if (
        e.events & EPOLLERR
        || e.events & EPOLLHUP
        || e.events & EPOLLRDHUP)
    {
        handleFileDescriptorClose(e);
    }
}

void Forte::PDUPeerFileDescriptorEndpoint::DataIn(
    const size_t len, const char *buf)
{
    {
        AutoUnlockMutex lock(mReceiveMutex);
        lockedDataIn(len, buf);
    }

    callbackIfPDUReady();
}

void Forte::PDUPeerFileDescriptorEndpoint::lockedDataIn(
    const size_t len, const char *buf)
{
    FTRACE;
    try
    {
        //while (thisPDUWillOverflowBuffer())
        while (len + mCursor >= mBufSize
               && mBufSize + mBufStepSize > mBufMaxSize)
        {
            mBufferFullCondition.Wait();
        }

        while ((len + mCursor) >= mBufSize)
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
            hlog(HLOG_DEBUG,
                 "PDU new size %llu", (unsigned long long)mBufSize);
        }
    }
    catch (std::bad_alloc &e)
    {
        throw EPeerBufferOutOfMemory();
    }
    memcpy(mPDUBuffer.get() + mCursor, buf, len);
    hlog(HLOG_DEBUG2, "received data; oldCursor=%llu newCursor=%llu",
         (unsigned long long) mCursor, (unsigned long long) mCursor + len);
    mCursor += len;
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

void Forte::PDUPeerFileDescriptorEndpoint::handleFileDescriptorClose(
    const struct epoll_event& e)
{
    // default FileDescriptorEndpoint implemenation has no recourse
    // except to give up and issue the call back
    FTRACE;
    AutoUnlockMutex fdlock(mFDLock);
    AutoUnlockMutex lock(mEPollLock);

    lockedRemoveFDFromEPoll();
    close(mFD);
    mFD = -1;

    PDUPeerEventPtr event(new PDUPeerEvent());
    event->mEventType = PDUPeerDisconnectedEvent;
    if (mEventCallback)
        mEventCallback(event);
}

void Forte::PDUPeerFileDescriptorEndpoint::TeardownEPoll()
{
    FTRACE;
    AutoUnlockMutex lock(mEPollLock);
    lockedRemoveFDFromEPoll();
    mEPollFD = -1;
}

void Forte::PDUPeerFileDescriptorEndpoint::lockedAddFDToEPoll()
{
    FTRACE;

    if (mFD != -1 && mEPollFD != -1)
    {
        hlog(HLOG_DEBUG2, "add fd %d to epoll fd %d", (int) mFD, mEPollFD);
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
                        (int) mEPollFD,
                        (int) mFD,
                        SystemCallUtil::GetErrorDescription(errno).c_str()));
        }
    }
}

void Forte::PDUPeerFileDescriptorEndpoint::lockedRemoveFDFromEPoll()
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
    FTRACE;
    AutoUnlockMutex lock(mSendMutex);
    size_t offset = 0;
    size_t len = sizeof(Forte::PDUHeader) + pdu.GetPayloadSize();
    hlog(HLOG_DEBUG2, "sending %zu bytes on fd %d, payloadsize = %u",
         len, mFD.GetFD(), pdu.GetPayloadSize());

    // Put PDU header & payload contents into single buffer for sending
    boost::shared_array<char> sendBuf = PDU::CreateSendBuffer(pdu);

    int flags = MSG_NOSIGNAL;
    while (len > 0)
    {
        int sent = send(mFD, sendBuf.get()+offset, len, flags);
        if (sent < 0)
            hlog_and_throw(
                HLOG_DEBUG,
                EPeerSendFailed(
                    FStringFC(),
                    "%s",
                    SystemCallUtil::GetErrorDescription(errno).c_str()));
        offset += sent;
        len -= sent;
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
    Forte::PDUHeader *pduHeader = (Forte::PDUHeader*)mPDUBuffer.get();

    // Validate PDU
    if (pduHeader->version != Forte::PDU::PDU_VERSION)
    {
        hlog(HLOG_DEBUG2, "invalid PDU version");
        throw EPDUVersionInvalid();
    }
    // \TODO figure out how to do proper opcode validation

    if (mCursor >= minPDUSize + pduHeader->payloadSize)
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
    size_t len = sizeof(Forte::PDUHeader) + pduHeader->payloadSize;;
    hlog(HLOG_DEBUG2, "found valid PDU: len=%zu", len);
    // \TODO this memmove() based method is inefficient.  Implement a
    // proper ring-buffer.
    out.SetHeader(*pduHeader);
    out.SetPayload(out.GetPayloadSize(),
                   mPDUBuffer.get()+sizeof(Forte::PDUHeader));

    // now, move the rest of the buffer and cursor back by 'len' bytes
    memmove(mPDUBuffer.get(), mPDUBuffer.get() + len, mBufSize - len);
    memset(mPDUBuffer.get() + mBufSize - len, 0, len);
    hlog(HLOG_DEBUG2, "found valid PDU: oldCursor=%zu newCursor=%zu",
         mCursor, mCursor - len);
    mCursor -= len;

    mBufferFullCondition.Signal();
    return true;
}
