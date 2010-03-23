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

        inline int post(void) {
            AutoUnlockMutex lock(mLock);
            if (++mValue > 0)
                mNotify.signal();
            return 0;
        };
        inline int wait(void) {
            AutoUnlockMutex lock(mLock);
            while (mValue <= 0)
                mNotify.wait();
            --mValue;
            return 0;
        };
        inline int trywait(void) {
            AutoUnlockMutex lock(mLock);
            if (mValue <= 0) { errno = EAGAIN; return -1; }
            --mValue;
            return 0;
        };
        inline int getvalue(void) {
            AutoUnlockMutex lock(mLock);
            return mValue;
        };

    protected:
        int mValue;
        Mutex mLock;
        ThreadCondition mNotify;
    };
};
#endif
