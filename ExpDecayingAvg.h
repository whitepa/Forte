// An exponentially decaying average... 
// useful for keeping statistics in the form of events per second
// where the rates are highly variable.

#ifndef __ExpDecayingAvg_h
#define __ExpDecayingAvg_h

#include <set>

#define UPDATE_DELAY 250 // each object will be updated every 250ms 

namespace Forte
{
    class CExpDecayingAvgThread;

    class CExpDecayingAvg : public Object {
        friend class CExpDecayingAvgThread;
    public:
        // damping time in ms
        CExpDecayingAvg(int dampingTime = 10000); // default 10 second damping
        virtual ~CExpDecayingAvg();
    
        // shutdown the update thread
        static void shutdown(void);

        // reset the average to zero
        void reset(void);
    
        // set the current input value (will remain set until reset() or increment())
        float set(float input);
        // increment the current input value (will be reset when avg is updated)
        float increment(float amount);
        // get the current value of the average, do not update.  interval in ms
        inline float get(int interval) { return mLastAvg * (interval / UPDATE_DELAY); }

    protected:
        // update the average using the current input value
        void update(void);

        static CMutex sThrMutex;
        static CExpDecayingAvgThread *sThread;
        static std::set<CExpDecayingAvg*> sObjs;
    
        int mDampingTime;
        struct timeval mLastUpdate;
        float mLastAvg;
        float mInput;
        bool mResetInputUponUpdate;
        CMutex mLock;
    };

    class CExpDecayingAvgThread : public CThread {
    public:
        inline CExpDecayingAvgThread() { initialized(); }
    protected:
        virtual void * run(void);
    };
};
#endif
