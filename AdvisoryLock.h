#ifndef __advisoryLock_h
#define __advisoryLock_h

#include "Types.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
// -----------------------------------------------------------------------------

namespace Forte {

class AdvisoryLock
{
public:
    AdvisoryLock(int fd, off64_t start, off64_t len, short whence = SEEK_SET);
    
    /// getLock returns a lock description equivalent to the lock
    /// currently blocking us.
    AdvisoryLock getLock(bool exclusive = false);

    /// sharedLock will return true on success, false if the lock failed
    ///
    bool sharedLock(bool wait = true);
    
    /// exclusiveLock will return true on success, false if the lock failed
    ///
    bool exclusiveLock(bool wait = true);

    /// unlock will remove the current lock
    ///
    void unlock(void);

protected:
    struct flock m_lock;
    int m_fd;
};
// -----------------------------------------------------------------------------

class AdvisoryAutoUnlock
{
public:
    AdvisoryAutoUnlock(int fd, off64_t start, off64_t len, bool exclusive, 
                       short whence = SEEK_SET) :
        m_advisoryLock(fd, start, len, whence)
        {
            if (exclusive)
                m_advisoryLock.exclusiveLock();
            else
                m_advisoryLock.sharedLock();
        }
    virtual ~AdvisoryAutoUnlock() { m_advisoryLock.unlock(); }
protected:
    AdvisoryLock m_advisoryLock;
};
// -----------------------------------------------------------------------------

}; /* namespace Forte{} */

#endif /* __advisoryLock_h */
