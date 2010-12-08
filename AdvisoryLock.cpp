// AdvisoryLock.cpp

#include <sys/time.h>

#include "AdvisoryLock.h"
#include "LogManager.h"

using namespace Forte;
// -----------------------------------------------------------------------------

AdvisoryLock::AdvisoryLock(int fd, off64_t start, off64_t len, short whence)
: m_fd(fd)
{
    m_lock.l_whence = whence;
    m_lock.l_start = start;
    m_lock.l_len = len;
}

/// getLock gets the first lock that blocks the lock description
///
AdvisoryLock AdvisoryLock::GetLock(bool exclusive)
{
    AdvisoryLock lock(*this);
    if (exclusive)
        lock.m_lock.l_type = F_WRLCK;
    else
        lock.m_lock.l_type = F_RDLCK;

    if (fcntl(m_fd, F_GETLK, &lock) == -1)
    {
        hlog(HLOG_DEBUG, "Unable to obtain lock in getLock");
        hlog_errno(HLOG_DEBUG);
    }

    return lock;
}

/// sharedLock will return true on success, false if the lock failed
///
bool AdvisoryLock::SharedLock(bool wait)
{
    m_lock.l_type = F_RDLCK;
    if (fcntl(m_fd, wait ? F_SETLKW : F_SETLK, &m_lock) == -1)
    {
        hlog(HLOG_DEBUG, "Unable to obtain sharedLock");
        hlog_errno(HLOG_DEBUG);

        return false;
    }

    return true;
}

/// exclusiveLock will return true on success, false if the lock failed
///
bool AdvisoryLock::ExclusiveLock(bool wait)
{
    m_lock.l_type = F_WRLCK;
    if (fcntl(m_fd, wait ? F_SETLKW : F_SETLK, &m_lock) == -1)
    {
        hlog(HLOG_DEBUG, "Unable to obtain exclusiveLock");
        hlog_errno(HLOG_DEBUG);

        return false;
    }

    return true;
}

/// unlock will remove the current lock
///
void AdvisoryLock::Unlock(void)
{
    m_lock.l_type = F_UNLCK;
    if (fcntl(m_fd, F_SETLK, &m_lock) == -1)
    {
        hlog(HLOG_DEBUG, "Unable to obtain unlock");
        hlog_errno(HLOG_DEBUG);
    }
}
