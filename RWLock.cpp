// RWLock.cpp
#include "Forte.h"

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
void RWLock::_write_lock(const char *file, unsigned line)
{
    // lock for writing
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "write_lock() requested at %s:%d", file, line);
#endif

    // this blocks until all locks have cleared
    // other locks will block until this lock clears
    mMainLock.wait();

    // wait for read locks to clear
    mReadLockMutex.wait();
    mReadLockAtomic.wait();

    // set locked line and such
    mFile = file;
    mLine = line;
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "write_lock() obtained at %s:%d", file, line);
#endif
}


void RWLock::_write_unlock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "write_unlock() at %s:%d", file, line);
#endif
#ifdef XDEBUG
    FString err;

    if (mReadLockMutex.getvalue() != 0)
    {
        err.Format("write_unlock: no write lock held: %s:%u", mFile.c_str(), mLine);
        throw std::logic_error(err);
    }
#endif

    mReadLockMutex.post();
    mReadLockAtomic.post();
    mMainLock.post();
}


// atomically convert a write lock to a read lock
void RWLock::_write_unlock_read_lock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "write_unlock_read_lock() at %s:%d", file, line);
#endif
#ifdef XDEBUG
    FString err;

    // check write lock
    if (mMainLock.getvalue() != 0)
    {
        err.Format("write_unlock_read_lock: no write lock held: %s:%u", mFile.c_str(), mLine);
        throw std::logic_error(err);
    }

    if (mReadLockCount.getvalue() != 0)
    {
        err.Format("write_unlock_read_lock: read lock count already 0: %s:%u", mFile.c_str(), mLine);
        throw std::logic_error(err);
    }
#endif

    mReadLockCount.post();
    mReadLockAtomic.post();
    mMainLock.post();
}


void RWLock::_read_lock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "read_lock() requested at %s:%d", file, line);
#endif
    // read lock
    mMainLock.wait();
    mReadLockAtomic.wait();
    if (mReadLockCount.getvalue() == 0) mReadLockMutex.wait();
    mReadLockCount.post();
    mReadLockAtomic.post();
    mFile = file;
    mLine = line;
    mMainLock.post();
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "read_lock() obtained at %s:%d", file, line);
#endif
}


bool RWLock::_read_trylock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "read_trylock() requested at %s:%d", file, line);
#endif
    // if the lock is available, read lock it, return false
    // if not, do not block and return true
    if (mMainLock.trywait()) return true;
    mReadLockAtomic.wait();
    if (mReadLockCount.getvalue() == 0) mReadLockMutex.wait();
    mReadLockCount.post();
    mReadLockAtomic.post();
    mFile = file;
    mLine = line;
    mMainLock.post();
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "read_trylock() obtained at %s:%d", file, line);
#endif
    return false;
}


void RWLock::_read_unlock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "read_unlock() at %s:%d", file, line);
#endif
#ifdef XDEBUG
    FString err;
#endif

    mReadLockAtomic.wait();

#ifdef XDEBUG
    if (mReadLockCount.getvalue() == 0)
    {
        err.Format("read_unlock: no read lock held: %s:%u", mFile.c_str(), mLine);
        mReadLockAtomic.post();
        throw std::logic_error(err);
    }
#endif

    mReadLockCount.trywait();
    if (mReadLockCount.getvalue() == 0) mReadLockMutex.post();
    mReadLockAtomic.post();
}
