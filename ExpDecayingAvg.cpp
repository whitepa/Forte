#include "Context.h"
#include "ExpDecayingAvg.h"
#include "LogManager.h"
#include "RunLoop.h"
#include "FTrace.h"
#include <boost/bind.hpp>

using namespace Forte;
using namespace boost;

ExpDecayingAvg::ExpDecayingAvg(const boost::shared_ptr<Forte::RunLoop> &rl,
                               int mode, int dampingTime) :
    mRunLoopPtr(rl), mMode(mode), mDampingTime(dampingTime)
{
    FTRACE;
    mDataPtr.reset(new ExpDecayingAvgData(mMode, mDampingTime));
    Timer::Callback callback = boost::bind(&ExpDecayingAvgData::update, mDataPtr);
    mTimerPtr.reset(new Timer("ExpDecayingAvgData::update", mRunLoopPtr, mDataPtr, callback,
                              Timespec::FromMillisec(UPDATE_DELAY),
                              true));
    mRunLoopPtr->AddTimer(mTimerPtr);
}
ExpDecayingAvg::~ExpDecayingAvg()
{
    FTRACE;
}

ExpDecayingAvgData::ExpDecayingAvgData(int mode, int dampingTime) :
    mMode(mode),
    mDampingTime(dampingTime)
{
    FTRACE;
    Reset();
}
ExpDecayingAvgData::~ExpDecayingAvgData()
{
    FTRACE;
}
void ExpDecayingAvgData::Reset(void)
{
    MonotonicClock mc;
    mLastUpdate = mc.GetTime();
    mLastAvg = 0.0;
    mLastRate = 0.0;
    mInput = 0.0;
    mLastInput = 0.0;
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
    // compute the difference between now and the input at last update
//    hlog(HLOG_DEBUG, "input=%f lastinput=%f lastavg=%f lastrate=%f",
//         mInput, mLastInput, mLastAvg, mLastRate);
    float diff = mInput - mLastInput;
    mLastInput = mInput;
    mLastRate += (1.0 - expf((float)-UPDATE_DELAY / (float)mDampingTime))*(diff - mLastRate);
    mLastAvg += (1.0 - expf((float)-UPDATE_DELAY / (float)mDampingTime))*(mInput - mLastAvg);
    if (mResetInputUponUpdate) mInput = 0.0;
}
