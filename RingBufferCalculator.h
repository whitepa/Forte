#ifndef __Forte_RingBufferCalculator__
#define __Forte_RingBufferCalculator__

#include <sys/epoll.h>
#include <sys/types.h>
#include <string.h>
#include <boost/shared_array.hpp>
#include "Exception.h"

// This ring buffer calculator is tightly coupled with
// PDUPeerEndpointFD. It is not thread safe and there has been no
// consideration into fitness for more general ring buffer
// calculations.
//
// of particular note is that consumers must handle a wrapping write,
// this calculator will tell you if a Write will wrap, and give you
// the length between the beginning and end of the write. also,
// although the calculator is similar to a traditional ring buffer, it
// is for use cases where *all* data needs to be processed, not just
// the most recent. also, write size will from the end of the valid
// data to the end of the buffer, or if the valid data is currently
// wrapped, the distance from the end of the valid data to the start
// of the valid data. that length is particularly suited to the length
// you want to pass to a recv() or read() call.


// ring buffer provides the information below for the various scenarios
// . = open space
// HH = PDUHeader (fixed
// p = payload (dynamic)
// o = optional data (dynamic)
//
// read cursor = 0, read size =  0, write cursor = 0, write size = 11, empty
// ...........
//
// read cursor = 0, read size =  4, write cursor = 4, write size = 7, !empty
// HHpo.......
//
// read cursor = 0, read size =  7, write cursor = 7, write size = 4, !empty
// HHpoHHp....
//
// read cursor = 0, read size = 11, write cursor = 0, write size = 0, !empty
// HHpoHHpHHpo
//
// read cursor = 4, read size =  7, write cursor = 0, write size = 4, !empty
// ....HHpHHpo
//
// read cursor = 7, read size =  4, write cursor = 0, write size = 7, !empty
// .......HHpo
//
// read cursor = 0, read size =  0, write cursor = 0, write size = 11, empty
// ...........
//
// read cursor = 3, read size =  3, write cursor = 6, write size = 5, !empty
// ...HHp.....
//
// read cursor = 10, read size = 3, write cursor = 2, write size = 8, !empty
// Hp........H

EXCEPTION_CLASS(ERingBufferCalculator);

#define RING_BUFFER_ASSERT(ASSERTION__, REASON__)                       \
    if (! (ASSERTION__) )                                               \
    {                                                                   \
        hlogstream(HLOG_ERR, "Attempt to write to full buffer "         \
                   << " size:" << mSize                                 \
                   << " writeCursor:" << mWriteCursor                   \
                   << " readCursor:" << mReadCursor                     \
                   << " full:" << mFull                                 \
                   << " readEpic:" << mReadEpic                         \
                   << " writeEpic:" << mWriteEpic);                     \
        throw ERingBufferCalculator("Attempt to write to full buffer"); \
                                                                        \
    }

class RingBufferCalculator
{
public:
    RingBufferCalculator(
        char* bufferAddress,
        size_t bufferSize)
        : mBufferAddress(bufferAddress),
          mSize(bufferSize),
          mWriteCursor(0),
          mReadCursor(0),
          mFull(false),
          mReadEpic(0),
          mWriteEpic(0)
        {}

    void Reset(char* bufferAddress,
               size_t bufferSize) {
        mBufferAddress = bufferAddress;
        mSize = bufferSize;
        mWriteCursor = 0;
        mReadCursor = 0;
        mFull = false;
        mReadEpic = 0;
        mWriteEpic = 0;
    }

    char* GetWriteLocation() const {
        RING_BUFFER_ASSERT(!mFull, "Attempt to write to full buffer");
        return mBufferAddress + mWriteCursor;
    }

    //TODO: in the std library, empty() is generally the cheaper
    //option, so switch the internal semantic.
    bool Empty() const {
        return !mFull && mWriteCursor == mReadCursor;
    }

    bool Full() const {
        return mFull;
    }

    const char* GetReadLocation() const {
        RING_BUFFER_ASSERT(!Empty(), "Attempt to read from empty buffer");
        return mBufferAddress + mReadCursor;
    }

    size_t GetWriteLength() const {
        if (mFull)
        {
            return 0;
        }
        else if (mWriteCursor >= mReadCursor)
        {
            return mSize - mWriteCursor;
        }
        else
        {
            return mReadCursor - mWriteCursor;
        }
    }

    size_t GetReadLength() const {
        if (mFull)
        {
            return mSize;
        }
        else if (mWriteCursor >= mReadCursor)
        {
            return mWriteCursor - mReadCursor;
        }
        else
        {
            return (mSize - mReadCursor) + mWriteCursor;
        }
    }

    size_t GetReadLengthNoWrap() const {
        if (Empty())
        {
            return 0;
        }
        else if (mReadCursor >= mWriteCursor)
        {
            // here to end of buffer
            return mSize - mReadCursor;
        }
        else
        {
            return mWriteCursor - mReadCursor;
        }
    }

    void RecordWrite(size_t len) {
        RING_BUFFER_ASSERT(len > 0, "Attempt to record empty write");
        RING_BUFFER_ASSERT(len <= GetWriteLength(),
                           "Attemt to write past end of buffer");

        mWriteCursor += len;
        // consumers must handle a wrapping write by utilizing
        // ObjectWillWrap, GetReadLength and and GetReadLengthNoWrap
        RING_BUFFER_ASSERT(mWriteCursor <= mSize,
                           "Attempt to record a wrapped write");
        if (mWriteCursor == mSize)
        {
            mWriteCursor = 0;
            ++mWriteEpic;
        }
        if (mReadCursor == mWriteCursor)
        {
            mFull = true;
        }
    }

    void RecordRead(size_t len) {
        RING_BUFFER_ASSERT(len > 0, "Attempt to read 0");
        RING_BUFFER_ASSERT(len <= GetReadLength(),
                           "Attempt to read more than is available");

        mReadCursor += len;
        if (mReadCursor >= mSize)
        {
            mReadCursor -= mSize;
            ++mReadEpic;
        }
        mFull = false;
    }

    bool ObjectWillWrap(size_t objectLength) const {
        return mReadCursor + objectLength > mSize;
    }

    void ObjectCopy(char* object, size_t objectLength) const {
        RING_BUFFER_ASSERT(objectLength <= GetReadLength(),
                           "Attempt to read object greater than buffer size");

        if (ObjectWillWrap(objectLength))
        {
            memcpy(object,
                   GetReadLocation(),
                   GetReadLengthNoWrap());

            memcpy(object + GetReadLengthNoWrap(),
                   mBufferAddress,
                   objectLength - GetReadLengthNoWrap());
        }
        else
        {
            memcpy(object, GetReadLocation(), objectLength);
        }
    }

private:
    char* mBufferAddress;
    size_t mSize;
    size_t mWriteCursor;
    size_t mReadCursor;
    bool mFull;
    int64_t mReadEpic;
    int64_t mWriteEpic;
};

#endif /* __Forte_RingBufferCalculator__ */
