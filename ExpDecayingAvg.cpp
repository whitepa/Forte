#include "Context.h"
#include "ExpDecayingAvg.h"
#include "RunLoop.h"
#include <boost/bind.hpp>

using namespace Forte;

ExpDecayingAvg::ExpDecayingAvg(Context &context, int dampingTime) :
    mContext(context)
{
    mDataPtr.reset(new ExpDecayingAvgData(dampingTime));
    Timer::Callback callback = boost::bind(&ExpDecayingAvgData::update, mDataPtr.get());
    mTimerPtr.reset(new Timer(mContext.Get<RunLoop>("forte.RunLoop"),
                              mDataPtr,
                              callback,
                              Timespec::FromMillisec(UPDATE_DELAY),
                              true));
}
ExpDecayingAvg::~ExpDecayingAvg()
{
}

ExpDecayingAvgData::ExpDecayingAvgData(int dampingTime) :
    mDampingTime(dampingTime) 
{ 
    Reset();
}
void ExpDecayingAvgData::Reset(void)
{
    MonotonicClock mc;
    mLastUpdate = mc.GetTime();
    mLastAvg = 0.0;
    mInput = 0.0;
    mResetInputUponUpdate = true;
}
float ExpDecayingAvgData::Set(float input)
{
    AutoUnlockMutex lock(mLock);
    mInput = input;
    mResetInputUponUpdate = false;
    return mLastAvg;
}
float ExpDecayingAvgData::Increment(float amount)
{
    AutoUnlockMutex lock(mLock);
    mInput += amount;
    mResetInputUponUpdate = true;
    return mLastAvg;
}
void ExpDecayingAvgData::update(void)
{
    // called by the update thread to recompute the average
    AutoUnlockMutex lock(mLock);
    // \TODO replace UPDATE_DELAY here with monotonic now - mLastUpdate.
    // then update mLastUpdate.
    mLastAvg += (1.0 - expf((float)-UPDATE_DELAY / (float)mDampingTime))*(mInput - mLastAvg);
    if (mResetInputUponUpdate) mInput = 0.0;
}
