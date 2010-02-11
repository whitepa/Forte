// RWLock.cpp
#include "Forte.h"

// ctor
RWLock::RWLock()
:
    m_main_lock(1),
    m_read_lock_mutex(1),
    m_read_lock_atomic(1),
    m_read_lock_count(0)
{
    m_line = 0;
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
    m_main_lock.wait();

    // wait for read locks to clear
    m_read_lock_mutex.wait();
    m_read_lock_atomic.wait();

    // set locked line and such
    m_file = file;
    m_line = line;
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

    if (m_read_lock_mutex.getvalue() != 0)
    {
        err.Format("write_unlock: no write lock held: %s:%u", m_file.c_str(), m_line);
        throw std::logic_error(err);
    }
#endif

    m_read_lock_mutex.post();
    m_read_lock_atomic.post();
    m_main_lock.post();
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
    if (m_main_lock.getvalue() != 0)
    {
        err.Format("write_unlock_read_lock: no write lock held: %s:%u", m_file.c_str(), m_line);
        throw std::logic_error(err);
    }

    if (m_read_lock_count.getvalue() != 0)
    {
        err.Format("write_unlock_read_lock: read lock count already 0: %s:%u", m_file.c_str(), m_line);
        throw std::logic_error(err);
    }
#endif

    m_read_lock_count.post();
    m_read_lock_atomic.post();
    m_main_lock.post();
}


void RWLock::_read_lock(const char *file, unsigned line)
{
#ifdef DEBUG_RWLOCK
    hlog(HLOG_DEBUG, "read_lock() requested at %s:%d", file, line);
#endif
    // read lock
    m_main_lock.wait();
    m_read_lock_atomic.wait();
    if (m_read_lock_count.getvalue() == 0) m_read_lock_mutex.wait();
    m_read_lock_count.post();
    m_read_lock_atomic.post();
    m_file = file;
    m_line = line;
    m_main_lock.post();
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
    if (m_main_lock.trywait()) return true;
    m_read_lock_atomic.wait();
    if (m_read_lock_count.getvalue() == 0) m_read_lock_mutex.wait();
    m_read_lock_count.post();
    m_read_lock_atomic.post();
    m_file = file;
    m_line = line;
    m_main_lock.post();
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

    m_read_lock_atomic.wait();

#ifdef XDEBUG
    if (m_read_lock_count.getvalue() == 0)
    {
        err.Format("read_unlock: no read lock held: %s:%u", m_file.c_str(), m_line);
        m_read_lock_atomic.post();
        throw std::logic_error(err);
    }
#endif

    m_read_lock_count.trywait();
    if (m_read_lock_count.getvalue() == 0) m_read_lock_mutex.post();
    m_read_lock_atomic.post();
}
