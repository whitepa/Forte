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
    EXCEPTION_CLASS(ERWLock);
    class RWLock : public Object
    {
    public:
        RWLock();
        virtual ~RWLock() { }

    public:
        void _WriteLock(const char *file, unsigned line);
        void _WriteUnlock(const char *file, unsigned line);
        void _WriteUnlockReadLock(const char *file, unsigned line);
        void _ReadLock(const char *file, unsigned line);
        bool _ReadTryLock(const char *file, unsigned line);
        void _ReadUnlock(const char *file, unsigned line);

    private:
        Semaphore mMainLock;
        Semaphore mReadLockMutex;
        Semaphore mReadLockAtomic;
        Semaphore mReadLockCount;
        FString mFile;
        unsigned mLine;
    };

#define WriteLock() _WriteLock(__FILE__, __LINE__)
#define WriteUnlock() _WriteUnlock(__FILE__, __LINE__)
#define WriteUnlockReadLock() _WriteUnlockReadLock(__FILE__, __LINE__)
#define ReadLock() _ReadLock(__FILE__, __LINE__)
#define ReadTryLock() _ReadTryLock(__FILE__, __LINE__)
#define ReadUnlock() _ReadUnlock(__FILE__, __LINE__)
    class AutoReadUnlock {
    public:
        AutoReadUnlock(RWLock &lock) : mUnlockOnDestruct(true), mLock(lock) { mLock.ReadLock(); }
        virtual ~AutoReadUnlock() { if(mUnlockOnDestruct) Unlock(); }
        inline void Unlock() { mLock.ReadUnlock(); Release(); }
        inline void Release() { mUnlockOnDestruct = false; }
    protected:
        bool mUnlockOnDestruct;
        RWLock &mLock;
    };
    template < class ExceptionClass >
    class CTryAutoReadUnlock {
    public:
        CTryAutoReadUnlock(RWLock &lock, ExceptionClass &e) : mUnlockOnDestruct(true), mLock(lock) { if (mLock.ReadTryLock()) throw e; }
        virtual ~CTryAutoReadUnlock() { if(mUnlockOnDestruct) Unlock(); }
        inline void Unlock() { mLock.ReadUnlock(); Release(); }
        inline void Release() { mUnlockOnDestruct = false; }
    protected:
        bool mUnlockOnDestruct;
        RWLock &mLock;
    };
    class AutoWriteUnlock {
    public:
        AutoWriteUnlock(RWLock &lock) : mUnlockOnDestruct(true), mLock(lock) { mLock.WriteLock(); }
        virtual ~AutoWriteUnlock() { if(mUnlockOnDestruct) Unlock(); }
        inline void Unlock() { mLock.WriteUnlock(); mUnlockOnDestruct = false; }
        inline void Release() { mUnlockOnDestruct = false; }
    protected:
        bool mUnlockOnDestruct;
        RWLock &mLock;
    };
};
#endif
