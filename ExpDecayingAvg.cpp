#include "Forte.h"

Mutex                          ExpDecayingAvg::sThrMutex;
ExpDecayingAvgThread*          ExpDecayingAvg::sThread = NULL;
std::set<ExpDecayingAvg*>      ExpDecayingAvg::sObjs;

ExpDecayingAvg::ExpDecayingAvg(int dampingTime) :
    mDampingTime(dampingTime)
{
    Reset();
    // create update thread if needed
    // one thread handles updates of all decaying avg objects
    if (sThread == NULL)
    {
        AutoUnlockMutex lock(sThrMutex);
        if (sThread == NULL)
        {
            sThread = new ExpDecayingAvgThread();
            // register a shutdown callback to delete the new thread
            ServerMain::GetServer().RegisterShutdownCallback(
                new CStaticCallback(&(ExpDecayingAvg::Shutdown)));
        }
    }
    AutoUnlockMutex lock(sThrMutex);
    sObjs.insert(this);
}
ExpDecayingAvg::~ExpDecayingAvg()
{
    AutoUnlockMutex lock(sThrMutex);
    sObjs.erase(this);
}
void ExpDecayingAvg::Shutdown(void)
{
    // static method to delete the update thread
    hlog(HLOG_DEBUG, "ExpDecayingAvg: update thread shutting down");
    AutoUnlockMutex lock(sThrMutex);
    if (sThread != NULL)
        delete sThread;
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

// this thread loops through all the existing objects and updates them
void * ExpDecayingAvgThread::run(void)
{
    // XXX note: this code (incorrectly) assumes that it takes zero time
    // to update all the averages.  As a result, the higher the number of averages
    // updated, the farther those numbers will be from correct.
    // Ideally, an absolute time for the next update should be computed, and
    // a timer set, rather than using usleep() with a fixed delay.
    mThreadName.Format("expdecay-%u", (unsigned)mThread);
    hlog(HLOG_DEBUG, "exponential decaying average updater thread starting up");
    while (!mThreadShutdown)
    {
        std::set<ExpDecayingAvg*>::iterator i;
        for (i = ExpDecayingAvg::sObjs.begin();
             i != ExpDecayingAvg::sObjs.end();
             ++i)
        {
            ExpDecayingAvg *obj = *i;
            if (obj != NULL)
            {
                obj->update();
            }
        }
        usleep(UPDATE_DELAY * 1000);
    }
    return NULL;
}
