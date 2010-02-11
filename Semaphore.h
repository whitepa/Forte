#ifndef __Semaphore_h
#define __Semaphore_h

#include <errno.h>
#include "AutoMutex.h"
#include "ThreadCondition.h"
namespace Forte
{
    class CSemaphore
    {
    public:
        inline CSemaphore(int value) : mValue(value), mNotify(mLock) {};
        inline int post(void) {
            CAutoUnlockMutex lock(mLock);
            if (++mValue > 0)
                mNotify.signal();
            return 0;
        };
        inline int wait(void) {
            CAutoUnlockMutex lock(mLock);
            while (mValue <= 0)
                mNotify.wait();
            --mValue;
            return 0;
        };
        inline int trywait(void) {
            CAutoUnlockMutex lock(mLock);
            if (mValue <= 0) { errno = EAGAIN; return -1; }
            --mValue;
            return 0;
        };
        inline int getvalue(void) {
            CAutoUnlockMutex lock(mLock);
            return mValue;
        };

    protected:
        int mValue;
        CMutex mLock;
        CThreadCondition mNotify;
    };
};
#endif
