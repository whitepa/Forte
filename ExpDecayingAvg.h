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

#define EDA_VALUE 0
#define EDA_RATE  1

namespace Forte
{
    class ExpDecayingAvgData : public Object
    {
    public:
        ExpDecayingAvgData(int mode, int dampingTime);
        virtual ~ExpDecayingAvgData();

        void Reset(void);
        inline float Get(void) { return mLastAvg; }
        inline float GetRate(int interval) { return mLastRate * (interval / UPDATE_DELAY); }
        float Set(float input);
        float Increment(float amount);
        void update(void);

        int mMode;
        int mDampingTime;
        Forte::Timespec mLastUpdate;
        float mLastAvg;
        float mLastRate;
        float mInput;
        float mLastInput;
        bool mResetInputUponUpdate;
        Forte::Mutex mLock;
    };

    class ExpDecayingAvg : public Object {
    public:
        /**
         * ExpDecayingAvg computes a decaying average of a particular
         * value, or the decaying average of a rate at which a value
         * is changing.
         *
         * MODES:
         *  EDA_VALUE - Decaying Average of the value should be computed.
         *  EDA_RATE  - Average rate of change of the value should be computed.
         *
         * @param context
         * @param mode which mode to operate in
         * @param dampingTime
         *
         * @return
         */
        ExpDecayingAvg(const boost::shared_ptr<Forte::RunLoop> &rl,
                       int mode, int dampingTime);
        virtual ~ExpDecayingAvg();

        // reset the average to zero
        void Reset(void) { mDataPtr->Reset(); }

        // set the current input value (will remain set until Reset() or increment())
        float Set(float input) { return mDataPtr->Set(input); }
        // increment the current input value (will be reset when avg is updated)
        float Increment(float amount) { return mDataPtr->Increment(amount); }
        // get the current value of the average, do not update
        inline float Get(void) { return mDataPtr->Get(); }
        inline float GetRate(int interval) { return mDataPtr->GetRate(interval); }

    protected:
        // update the average using the current input value
        void update(void);

        boost::shared_ptr<Forte::RunLoop> mRunLoopPtr;
        int mMode;
        int mDampingTime;
        boost::shared_ptr<ExpDecayingAvgData> mDataPtr;
        boost::shared_ptr<Forte::Timer> mTimerPtr;
    };

};
#endif
