// RWLock.cpp
#include "LogManager.h"
#include "RWLock.h"

using namespace Forte;

// ctor
RWLock::RWLock()
:
    mMainLock(1),
    mReadLockMutex(1),
    mReadLockAtomic(1),
    mReadLockCount(0)
{
    mLine = 0;
}


// write lock functions
void RWLock::_WriteLock(const char *file, unsigned line)
{
    // lock for writing
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "WriteLock() requested at %s:%d", file, line);
#endif

    // this blocks until all locks have cleared
    // other locks will block until this lock clears
    mMainLock.Wait();

    // wait for read locks to clear
    mReadLockMutex.Wait();
    mReadLockAtomic.Wait();

    // set locked line and such
    mFile = file;
    mLine = line;
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "WriteLock() obtained at %s:%d", file, line);
#endif
}


void RWLock::_WriteUnlock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "WriteUnlock() at %s:%d", file, line);
#endif
#ifdef XDEBUG
    FString err;

    if (mReadLockMutex.GetValue() != 0)
    {
        err.Format("WriteUnlock: no write lock held: %s:%u", mFile.c_str(), mLine);
        throw ERWLock(err);
    }
#endif

    mReadLockMutex.Post();
    mReadLockAtomic.Post();
    mMainLock.Post();
}


// atomically convert a write lock to a read lock
void RWLock::_WriteUnlockReadLock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "WriteUnlockReadLock() at %s:%d", file, line);
#endif
#ifdef XDEBUG
    FString err;

    // check write lock
    if (mMainLock.GetValue() != 0)
    {
        err.Format("WriteUnlockReadLock: no write lock held: %s:%u", mFile.c_str(), mLine);
        throw ERWLock(err);
    }

    if (mReadLockCount.GetValue() != 0)
    {
        err.Format("WriteUnlockReadLock: read lock count already 0: %s:%u", mFile.c_str(), mLine);
        throw ERWLock(err);
    }
#endif

    mReadLockCount.Post();
    mReadLockAtomic.Post();
    mMainLock.Post();
}


void RWLock::_ReadLock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "ReadLock() requested at %s:%d", file, line);
#endif
    // read lock
    mMainLock.Wait();
    mReadLockAtomic.Wait();
    if (mReadLockCount.GetValue() == 0) mReadLockMutex.Wait();
    mReadLockCount.Post();
    mReadLockAtomic.Post();
    mFile = file;
    mLine = line;
    mMainLock.Post();
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "ReadLock() obtained at %s:%d", file, line);
#endif
}


bool RWLock::_ReadTryLock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "ReadTryLock() requested at %s:%d", file, line);
#endif
    // if the lock is available, read lock it, return false
    // if not, do not block and return true
    if (mMainLock.TryWait()) return true;
    mReadLockAtomic.Wait();
    if (mReadLockCount.GetValue() == 0) mReadLockMutex.Wait();
    mReadLockCount.Post();
    mReadLockAtomic.Post();
    mFile = file;
    mLine = line;
    mMainLock.Post();
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "ReadTryLock() obtained at %s:%d", file, line);
#endif
    return false;
}


void RWLock::_ReadUnlock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "ReadUnlock() at %s:%d", file, line);
#endif
#ifdef XDEBUG
    FString err;
#endif

    mReadLockAtomic.Wait();

#ifdef XDEBUG
    if (mReadLockCount.GetValue() == 0)
    {
        err.Format("ReadUnlock: no read lock held: %s:%u", mFile.c_str(), mLine);
        mReadLockAtomic.Post();
        throw ERWLock(err);
    }
#endif

    mReadLockCount.TryWait();
    if (mReadLockCount.GetValue() == 0) mReadLockMutex.Post();
    mReadLockAtomic.Post();
}
