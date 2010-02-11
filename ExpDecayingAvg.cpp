#include "Forte.h"

CMutex                          CExpDecayingAvg::sThrMutex;
CExpDecayingAvgThread*          CExpDecayingAvg::sThread = NULL;
std::set<CExpDecayingAvg*>      CExpDecayingAvg::sObjs;

CExpDecayingAvg::CExpDecayingAvg(int dampingTime) :
    mDampingTime(dampingTime)
{
    reset();
    // create update thread if needed
    // one thread handles updates of all decaying avg objects
    if (sThread == NULL)
    {
        CAutoUnlockMutex lock(sThrMutex);
        if (sThread == NULL)
        {
            sThread = new CExpDecayingAvgThread();
            // register a shutdown callback to delete the new thread
            CServerMain::GetServer().RegisterShutdownCallback(
                new CStaticCallback(&(CExpDecayingAvg::shutdown)));
        }
    }
    CAutoUnlockMutex lock(sThrMutex);
    sObjs.insert(this);
}
CExpDecayingAvg::~CExpDecayingAvg()
{
    CAutoUnlockMutex lock(sThrMutex);
    sObjs.erase(this);
}
void CExpDecayingAvg::shutdown(void)
{
    // static method to delete the update thread
    hlog(HLOG_DEBUG, "ExpDecayingAvg: update thread shutting down");
    CAutoUnlockMutex lock(sThrMutex);
    if (sThread != NULL)
        delete sThread;
}
void CExpDecayingAvg::reset(void)
{
    CAutoUnlockMutex lock(mLock);
    gettimeofday(&mLastUpdate, NULL);
    mLastAvg = 0.0;
    mInput = 0.0;
    mResetInputUponUpdate = true;
}
void CExpDecayingAvg::update(void)
{
    // called by the update thread to recompute the average
    CAutoUnlockMutex lock(mLock);
    mLastAvg += (1.0 - expf((float)-UPDATE_DELAY / (float)mDampingTime))*(mInput - mLastAvg);
    if (mResetInputUponUpdate) mInput = 0.0;
}
float CExpDecayingAvg::set(float input)
{
    CAutoUnlockMutex lock(mLock);
    mInput = input;
    mResetInputUponUpdate = false;
    return mLastAvg;
}
float CExpDecayingAvg::increment(float amount)
{
    CAutoUnlockMutex lock(mLock);
    mInput += amount;
    mResetInputUponUpdate = true;
    return mLastAvg;
}

// this thread loops through all the existing objects and updates them
void * CExpDecayingAvgThread::run(void)
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
        std::set<CExpDecayingAvg*>::iterator i;
        for (i = CExpDecayingAvg::sObjs.begin();
             i != CExpDecayingAvg::sObjs.end();
             ++i)
        {
            CExpDecayingAvg *obj = *i;
            if (obj != NULL)
            {
                obj->update();
            }
        }
        usleep(UPDATE_DELAY * 1000);
    }
    return NULL;
}
