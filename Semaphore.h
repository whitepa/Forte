#ifndef __Semaphore_h
#define __Semaphore_h

#include <errno.h>
#include "Object.h"
#include "AutoMutex.h"
#include "ThreadCondition.h"
namespace Forte
{
    class Semaphore : public Object
    {
    public:
        inline Semaphore(int value) : mValue(value), mNotify(mLock) {};
        virtual ~Semaphore() {};

        inline int Post(int num = 1) {
            AutoUnlockMutex lock(mLock);
            mValue+=num;
            if (mValue > 0)
                mNotify.Signal();
            return 0;
        };
        inline int Wait(void) {
            AutoUnlockMutex lock(mLock);
            while (mValue <= 0)
                mNotify.Wait();
            --mValue;
            return 0;
        };
        inline int TryWait(void) {
            AutoUnlockMutex lock(mLock);
            if (mValue <= 0) { errno = EAGAIN; return -1; }
            --mValue;
            return 0;
        };
        inline int GetValue(void) {
            AutoUnlockMutex lock(mLock);
            return mValue;
        };

    protected:
        int mValue;
        Mutex mLock;
        ThreadCondition mNotify;
    };;
};
#endif
