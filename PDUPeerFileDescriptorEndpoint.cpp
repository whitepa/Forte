// #SCQAD TAG: forte.pdupeer
#include <sys/epoll.h>
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
        //TODO: differently
        char recvBuffer[5000];
        size_t bufferSize = 5000;
        // input is ready
        int len = recv(mFD, recvBuffer, bufferSize, 0);
        if (len < 0)
        {
            // TODO:
            // EINTR

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
            hlog(HLOG_DEBUG2, "received %d bytes on fd %d", len, mFD.Fd());
            DataIn(len, recvBuffer);
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

void Forte::PDUPeerFileDescriptorEndpoint::DataIn(size_t len, char *buf)
{
    FTRACE;
    {
        AutoUnlockMutex lock(mFDLock);
        try {
            while ((len + mCursor) >= mBufSize) {
                size_t newsize = mBufSize + mBufStepSize;
                if (newsize > mBufMaxSize)
                    throw EPeerBufferOverflow();
                char *tmpstr = new char[newsize];
                memset (tmpstr, 0, newsize);
                if (mBufSize)
                    memcpy(tmpstr, mPDUBuffer.get(), mBufSize);
                mPDUBuffer.reset(tmpstr);
                mBufSize = newsize;
                hlog(HLOG_DEBUG, "PDU new size %llu", (u64)mBufSize);
            }
        } catch (std::bad_alloc &e) {
            throw EPeerBufferOutOfMemory();
        }
        memcpy(mPDUBuffer.get() + mCursor, buf, len);
        hlog(HLOG_DEBUG2, "received data; oldCursor=%llu newCursor=%llu",
             (u64) mCursor, (u64) mCursor + len);
        mCursor += len;
    }

    // i think there is technically a race here where you could miss a
    // PDU if a PDU is ready before mPDUReceivedCallbackFunc is set
    // and it is the last PDU but I can't think of a case where it
    // matters
    if (mEventCallback && IsPDUReady())
    {
        PDUPeerEventPtr event(new PDUPeerEvent());
        event->mEventType = PDUPeerReceivedPDUEvent;
        mEventCallback(event);
    }
}

void Forte::PDUPeerFileDescriptorEndpoint::SendPDU(const Forte::PDU &pdu)
{
    FTRACE;
    AutoUnlockMutex lock(mFDLock);
    size_t len = sizeof(Forte::PDU) - Forte::PDU::PDU_MAX_PAYLOAD;
    len += pdu.payloadSize;
    if (len > sizeof(Forte::PDU))
        throw EPeerSendInvalid();
    hlog(HLOG_DEBUG2, "sending %zu bytes on fd %d, payloadsize = %u",
            len, mFD.GetFD(), pdu.payloadSize);

    size_t offset = 0;
    const char* buf = reinterpret_cast<const char*>(&pdu);
    int flags = MSG_NOSIGNAL;
    while (len > 0)
    {
        int sent = send(mFD, buf+offset, len, flags);
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
    AutoUnlockMutex lock(mFDLock);
    return lockedIsPDUReady();
}

bool Forte::PDUPeerFileDescriptorEndpoint::lockedIsPDUReady(void) const
{
    // check for a valid PDU
    // (for now, read cursor is always the start of buffer)
    size_t minPDUSize = sizeof(Forte::PDU) - Forte::PDU::PDU_MAX_PAYLOAD;
    if (mCursor < minPDUSize)
    {
        hlog(HLOG_DEBUG4, "buffer contains less data than minimum PDU %zu vs %zu",
             mCursor, minPDUSize);
        return false;
    }
    Forte::PDU *pdu = reinterpret_cast<Forte::PDU *>(mPDUBuffer.get());
    if (pdu->version != Forte::PDU::PDU_VERSION)
    {
        hlog(HLOG_DEBUG2, "invalid PDU version");
        throw EPDUVersionInvalid();
    }
    // \TODO figure out how to do proper opcode validation

    if (mCursor >= minPDUSize + pdu->payloadSize)
        return true;
    else
        return false;
}

bool Forte::PDUPeerFileDescriptorEndpoint::RecvPDU(Forte::PDU &out)
{
    AutoUnlockMutex lock(mFDLock);
    if (!lockedIsPDUReady())
        return false;
    size_t minPDUSize = sizeof(Forte::PDU) - Forte::PDU::PDU_MAX_PAYLOAD;
    Forte::PDU *pdu = reinterpret_cast<Forte::PDU *>(mPDUBuffer.get());
    size_t len = minPDUSize + pdu->payloadSize;
    hlog(HLOG_DEBUG2, "found valid PDU: len=%zu", len);
    // \TODO this memmove() based method is inefficient.  Implement a
    // proper ring-buffer.
    memcpy(&out, pdu, len);
    // now, move the rest of the buffer and cursor back by 'len' bytes
    memmove(mPDUBuffer.get(), mPDUBuffer.get() + len, mBufSize - len);
    memset(mPDUBuffer.get() + mBufSize - len, 0, len);
    hlog(HLOG_DEBUG2, "found valid PDU: oldCursor=%zu newCursor=%zu",
         mCursor, mCursor - len);
    mCursor -= len;
    return true;
}
