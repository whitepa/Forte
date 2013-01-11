#include "IOManager.h"

using namespace Forte;

IORequest::IORequest(const boost::shared_ptr<IOManager>& ioManager)
    : mIOManager(ioManager),
      mCompleted(false),
      mResult(0),
      mData(NULL),
      mWaitCond(mWaitLock)
{
    memset(&mIOCB, 0, sizeof(struct iocb));
    mIOCBs[0] = &mIOCB;
}

void IORequest::Begin()
{
    mIOManager.lock()->SubmitRequest(shared_from_this());
}

void IORequest::SetOp(IORequest::OperationType op)
{
    switch (op) {
    case READ:
        mIOCB.aio_lio_opcode = IO_CMD_PREAD;
        break;
    case WRITE:
        mIOCB.aio_lio_opcode = IO_CMD_PWRITE;
        break;
    default:
        break;
    }
}

void IORequest::SetBuffer(void *buf, size_t len)
{
    FTRACE2("buf=%p len=%lu", buf, len);
    mIOCB.u.c.buf = buf;
    mIOCB.u.c.nbytes = len;
}

void IORequest::SetOffset(off_t off)
{
    mIOCB.u.c.offset = off;
}

void IORequest::SetFD(int fd)
{
    mIOCB.aio_fildes = fd;
}

void IORequest::SetCallback(IOCompletionCallback cb)
{
    mCallback = cb;
}

void IORequest::SetUserData(void *data)
{
    mData = data;
}

void* IORequest::GetUserData(void) const
{
    return mData;
}

// Called only by IOManager
void IORequest::SetRequestNumber(uint64_t reqNum)
{
    mIOCB.data = (void *)reqNum;
}

uint64_t IORequest::GetRequestNumber(void) const
{
    return (uint64_t)mIOCB.data;
}

struct iocb ** IORequest:: GetIOCBs(void)
{
    return mIOCBs;
}

void IORequest::SetResult(long res)
{
    FTRACE2("res=%ld", res);
    AutoUnlockMutex lock(mWaitLock);
    mResult = res;
    mCompleted = true;
    mWaitCond.Broadcast();
    if (mCallback)
        mCallback(*this);
}

long IORequest::GetResult(void) const
{
    if (!mCompleted)
        throw EIORequestStillInProgress();
    return mResult;
}

void IORequest::Wait(void)
{
    // wait for the IO completion

    // @TODO look into the efficiency of using pthread
    // condition variables for this.  We may want to switch to
    // a futex.  In particular, the current implementation
    // instantiates and initializes a pthread_mutex and
    // pthread_cond_t for each IO request.

    Forte::AutoUnlockMutex lock(mWaitLock);
    while (!mCompleted)
        mWaitCond.Wait();
}

bool IORequest::IsComplete(void) const
{
    return mCompleted;
}


