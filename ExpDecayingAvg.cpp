#include "Forte.h"

using namespace Forte;

ExpDecayingAvg::ExpDecayingAvg(Context &context, int dampingTime) :
    mContext(context),
    mDampingTime(dampingTime)
{
    Reset();
    // Register this object with a run loop
}
ExpDecayingAvg::~ExpDecayingAvg()
{
}
void ExpDecayingAvg::Reset(void)
{
    AutoUnlockMutex lock(mLock);
    gettimeofday(&mLastUpdate, NULL);
    mLastAvg = 0.0;
    mInput = 0.0;
    mResetInputUponUpdate = true;
}
void ExpDecayingAvg::update(void)
{
    // called by the update thread to recompute the average
    AutoUnlockMutex lock(mLock);
    mLastAvg += (1.0 - expf((float)-UPDATE_DELAY / (float)mDampingTime))*(mInput - mLastAvg);
    if (mResetInputUponUpdate) mInput = 0.0;
}
float ExpDecayingAvg::Set(float input)
{
    AutoUnlockMutex lock(mLock);
    mInput = input;
    mResetInputUponUpdate = false;
    return mLastAvg;
}
float ExpDecayingAvg::Increment(float amount)
{
    AutoUnlockMutex lock(mLock);
    mInput += amount;
    mResetInputUponUpdate = true;
    return mLastAvg;
}
