#include "AutoMutex.h"
#include "LogManager.h"
#include "PDUPeer.h"
#include "Types.h"

void Forte::PDUPeer::DataIn(size_t len, char *buf)
{
    AutoUnlockMutex lock(mLock);
    if ((mCursor + len) > mBufSize)
        throw EPeerBufferOverflow();
    memcpy(mPDUBuffer.get() + mCursor, buf, len);
    hlog(HLOG_DEBUG2, "received data; oldCursor=%llu newCursor=%llu",
         (u64) mCursor, (u64) mCursor + len);
    mCursor += len;
}

void Forte::PDUPeer::SendPDU(const Forte::PDU &pdu) const
{
    AutoUnlockMutex lock(mLock);
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
            throw EPeerSendFailed(FStringFC(), "%s", strerror(errno));
        offset += sent;
        len -= sent;
    }
}

bool Forte::PDUPeer::IsPDUReady(void) const
{
    AutoUnlockMutex lock(mLock);
    return lockedIsPDUReady();
}

bool Forte::PDUPeer::lockedIsPDUReady(void) const
{
    // returns true if a PDU has been received
    //         false otherwise
    // ('out' will be populated with the PDU if true)

    // check for a valid PDU
    // (for now, read cursor is always the start of buffer)
    size_t minPDUSize = sizeof(Forte::PDU) - Forte::PDU::PDU_MAX_PAYLOAD;
    if (mCursor < minPDUSize)
    {
        hlog(HLOG_DEBUG4, "buffer contains less data than minimum PDU");
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

bool Forte::PDUPeer::RecvPDU(Forte::PDU &out)
{
    AutoUnlockMutex lock(mLock);
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
