#ifndef __Forte_MonotonicCounter_h_
#define __Forte_MonotonicCounter_h_

namespace Forte
{
    //TODO: templatize
    class MonotonicCounter : public Object
    {
    public:
        MonotonicCounter(uint32_t start=1)
            : mCount(start)
            {}

        ~MonotonicCounter() {}

        uint32_t Next() {
            AutoUnlockMutex lock(mMutex);
            mCount++;
            return mCount - 1;
        }
    protected:
        Forte::Mutex mMutex;
        uint32_t mCount;
    };
    typedef boost::shared_ptr<MonotonicCounter> MonotonicCounterPtr;
};

#endif
