/**
 * An exponentially decaying average...  useful for keeping statistics
 * in the form of events per second where the rates are highly
 * variable.
 *
 * Usage of an ExpDecayingAvg object requires an event loop thread
 * present in the context.  The event loop thread is required for the
 * periodic computations required to maintain the decaying average.
 *
 */

#ifndef __ExpDecayingAvg_h
#define __ExpDecayingAvg_h

#include <set>
#include "Timer.h"

#define UPDATE_DELAY 250 // each object will be updated every 250ms 

namespace Forte
{
    class ExpDecayingAvgData : public Object
    {
    public:
        ExpDecayingAvgData(int dampingTime);

        void Reset(void);
        inline float Get(int interval) { return mLastAvg * (interval / UPDATE_DELAY); }
        float Set(float input);
        float Increment(float amount);
        void update(void);

        int mDampingTime;
        Timespec mLastUpdate;
        float mLastAvg;
        float mInput;
        bool mResetInputUponUpdate;
        Mutex mLock;
    };

    class ExpDecayingAvg : public Object {
    public:
        // damping time in ms
        ExpDecayingAvg(Forte::Context &context,
                       int dampingTime = 10000);
        virtual ~ExpDecayingAvg();
    
        // reset the average to zero
        void Reset(void);
    
        // set the current input value (will remain set until Reset() or increment())
        float Set(float input);
        // increment the current input value (will be reset when avg is updated)
        float Increment(float amount);
        // get the current value of the average, do not update.  interval in ms
        inline float Get(int interval) { return mDataPtr->Get(interval); }

    protected:
        // update the average using the current input value
        void update(void);

        Forte::Context &mContext;
        shared_ptr<ExpDecayingAvgData> mDataPtr;
        shared_ptr<Timer> mTimerPtr;
    };

};
#endif
