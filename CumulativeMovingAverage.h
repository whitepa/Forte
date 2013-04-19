#ifndef __Forte_CumulativeMovingAverage_h__
#define __Forte_CumulativeMovingAverage_h__

#include <stdint.h>

namespace Forte
{
    class CumulativeMovingAverage {
    public:
    CumulativeMovingAverage()
        : mNumberOfDataPoints (0),
        mAverage (0) {}

        ~CumulativeMovingAverage() {}

        CumulativeMovingAverage& operator=(int64_t value) {
            mAverage = ((mAverage * mNumberOfDataPoints) + value) / (mNumberOfDataPoints + 1);
            mNumberOfDataPoints++;
            return *this;
        }

        operator int64_t () { return mAverage; }

    private:
        uint64_t mNumberOfDataPoints;
        double mAverage;
    };
}

#endif
