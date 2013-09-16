#ifndef __Forte_MonotonicCounter_h_
#define __Forte_MonotonicCounter_h_

#include "Object.h"
#include "AutoMutex.h"

namespace Forte
{
    //TODO: templatize
    class MonotonicCounter : public Object
    {
    public:
        typedef uint32_t Int;

        MonotonicCounter(Int prev = 0) : mCount(prev) {}

        ~MonotonicCounter() {}

        operator Int() const
        {
            AutoUnlockMutex lock(mMutex);

            return mCount;
        }

        Int operator++()
        {
            AutoUnlockMutex lock(mMutex);

            return ++mCount;
        }

        Int operator++(int)
        {
            AutoUnlockMutex lock(mMutex);

            return mCount++;
        }

        Int Next()
        {
            return ++*this;
        }
    private:
        mutable Forte::Mutex mMutex;
        Int mCount;
    };
    typedef boost::shared_ptr<MonotonicCounter> MonotonicCounterPtr;
};

#endif
