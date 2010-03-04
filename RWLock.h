// RWLock.h
#ifndef __util_rwlock_h
#define __util_rwlock_h

#include <pthread.h>
#include "FString.h"
#include "Object.h"
#include "./Semaphore.h"
#include "./Exception.h"

namespace Forte
{
    class RWLock : public Object
    {
    public:
        RWLock();
        virtual ~RWLock() { }

    public:
        void _write_lock(const char *file, unsigned line);
        void _write_unlock(const char *file, unsigned line);
        void _write_unlock_read_lock(const char *file, unsigned line);
        void _read_lock(const char *file, unsigned line);
        bool _read_trylock(const char *file, unsigned line);
        void _read_unlock(const char *file, unsigned line);

    private:
        CSemaphore m_main_lock;
        CSemaphore m_read_lock_mutex;
        CSemaphore m_read_lock_atomic;
        CSemaphore m_read_lock_count;
        FString m_file;
        unsigned m_line;
    };

#define write_lock() _write_lock(__FILE__, __LINE__)
#define write_unlock() _write_unlock(__FILE__, __LINE__)
#define write_unlock_read_lock() _write_unlock_read_lock(__FILE__, __LINE__)
#define read_lock() _read_lock(__FILE__, __LINE__)
#define read_trylock() _read_trylock(__FILE__, __LINE__)
#define read_unlock() _read_unlock(__FILE__, __LINE__)
    class CAutoReadUnlock {
    public:
        CAutoReadUnlock(RWLock &lock) : mUnlockOnDestruct(true), mLock(lock) { mLock.read_lock(); }
        virtual ~CAutoReadUnlock() { if(mUnlockOnDestruct) unlock(); }
        inline void unlock() { mLock.read_unlock(); release(); }
        inline void release() { mUnlockOnDestruct = false; }
    protected:
        bool mUnlockOnDestruct;
        RWLock &mLock;
    };
    template < class ExceptionClass >
    class CTryAutoReadUnlock {
    public:
        CTryAutoReadUnlock(RWLock &lock, ExceptionClass &e) : mUnlockOnDestruct(true), mLock(lock) { if (mLock.read_trylock()) throw e; }
        virtual ~CTryAutoReadUnlock() { if(mUnlockOnDestruct) unlock(); }
        inline void unlock() { mLock.read_unlock(); release(); }
        inline void release() { mUnlockOnDestruct = false; }
    protected:
        bool mUnlockOnDestruct;
        RWLock &mLock;
    };
    class CAutoWriteUnlock {
    public:
        CAutoWriteUnlock(RWLock &lock) : mUnlockOnDestruct(true), mLock(lock) { mLock.write_lock(); }
        virtual ~CAutoWriteUnlock() { if(mUnlockOnDestruct) unlock(); }
        inline void unlock() { mLock.write_unlock(); mUnlockOnDestruct = false; }
        inline void release() { mUnlockOnDestruct = false; }
    protected:
        bool mUnlockOnDestruct;
        RWLock &mLock;
    };
};
#endif
