#ifndef __Forte__ExponentiallyDampedMovingAverage_h__
#define __Forte__ExponentiallyDampedMovingAverage_h__

#include "RunLoop.h"
#include <boost/bind.hpp>

namespace Forte
{
    class ExponentiallyDampedMovingAverage
    {
    private:
        class Data
        {
        public:
            Data(unsigned precision,
                 const Forte::Timespec &dampingInterval,
                 const Forte::Timespec &updateInterval,
                 boost::function<int64_t(void)> valueGetter) :
                mValue(0),
                mPrecision(precision),
                mUpdateInterval(updateInterval),
                mValueGetter(valueGetter) {
                double dampingRate =
                    static_cast<double>(updateInterval.AsMillisec()) /
                    dampingInterval.AsMillisec();
                mExp = static_cast<int64_t>(
                    static_cast<double>(1 << precision) *
                    (static_cast<double>(1.0) / exp(dampingRate)));
                hlogstream(HLOG_DEBUG3, "precision=" << mPrecision <<
                           " updateIntervalMillisec=" << updateInterval.AsMillisec() <<
                           " dampingIntervalMillisec=" << dampingInterval.AsMillisec() <<
                           " mExp=" << mExp <<
                           " dampingRate=" << dampingRate);
            }
            inline operator double () {
                Forte::AutoUnlockMutex lock(mLock);
                return static_cast<double>(mValue) / (1 << mPrecision);
            }
            inline double AsRate(const Forte::Timespec &per) {
                Forte::AutoUnlockMutex lock(mLock);
                return static_cast<double>(mValue) * per.AsMillisec() / mUpdateInterval.AsMillisec() / (1 << mPrecision);
            }
            void calc(void) {
                Forte::AutoUnlockMutex lock(mLock);
                int64_t current = mValueGetter() << mPrecision;
                mValue *= mExp;
                mValue += current * ((1ULL << mPrecision) - mExp);
                mValue >>= mPrecision;
            }
            Forte::Mutex mLock;
            int64_t mValue;
            uint64_t mPrecision;
            int64_t mExp;
            Forte::Timespec mUpdateInterval;
            boost::function<int64_t(void)> mValueGetter;
        };

    public:
        ExponentiallyDampedMovingAverage(unsigned precision,
                                         const Forte::Timespec &dampingInterval,
                                         const Forte::Timespec &updateInterval,
                                         boost::function<int64_t(void)> valueGetter,
                                         const boost::shared_ptr<Forte::RunLoop> &rl) :
            mRunLoop(rl) {
            mData.reset(new Data(precision,
                                 dampingInterval,
                                 updateInterval,
                                 valueGetter));
            Timer::Callback callback =
                boost::bind(&Data::calc, mData);
            mTimer.reset(new Timer("Data::calc", mRunLoop,
                                   callback, updateInterval, true));
            mRunLoop->AddTimer(mTimer);
        }
        inline operator double () { return *mData; }
        inline double AsRate(const Forte::Timespec &per) { return mData->AsRate(per); }
        boost::shared_ptr<Forte::RunLoop> mRunLoop;
        boost::shared_ptr<Data> mData;
        boost::shared_ptr<Forte::Timer> mTimer;
    };
};

#endif
