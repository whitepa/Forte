#ifndef __forte__EnableStats_h__
#define __forte__EnableStats_h__

#include <map>
#include "FString.h"
#include "Foreach.h"
#include "Exception.h"
#include "AutoMutex.h"
#include "LogManager.h"
#include "WeakFunctionBinder.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace Forte
{

    EXCEPTION_CLASS(EEnableStats);
    EXCEPTION_SUBCLASS2(EEnableStats, EFailedToFindStat,
                        "Failed to find stat");

    /**
     * To expose the Stats functions in Abstract base classes inherit
     * from BaseEnableStats. Make sure the use "public virtual" when
     * inheriting from BaseEnableStats.
     */
    class BaseEnableStats {
    public:
        template<typename T, typename L> friend class EnableStats;

        virtual std::map<FString, int64_t> GetAllStats(void) {
            // default impl to return empty
            return std::map<FString, int64_t>();
        }

        virtual int64_t GetStat(const Forte::FString &name) {
            boost::throw_exception(EFailedToFindStat(name));
        }

    protected:
        virtual void includeStatsFromChild(
            const boost::shared_ptr<BaseEnableStats> &child,
            const Forte::FString &childStatObjName) {
        }

    private:
        typedef boost::function<std::map<FString, int64_t> (void) > ChildStatCall;

        virtual void registerParent(
            boost::function<void()> parentUnregisterFn) {
        }

        virtual void registerChild(
            const Forte::FString &childStatObjName,
            ChildStatCall call) {
            // do nothing
        }

        virtual void removeChild(
            const Forte::FString &childStatObjName) {
            // do nothing
        }
    };


    /**
     * EnableStats injects stats functionality into the Derived class.
     * Derived is the child class name, LocalsType is templated Locals
     * class, see Locals.h for more info. To see an example of how
     * this class is used see ./unittest/EnableStatsUnitTest.cpp
     */
    template <typename Derived, typename LocalsType>
        class EnableStats : public virtual BaseEnableStats
    {
    public:
        EnableStats() {}

        virtual ~EnableStats() {
            removeChildFromParent();
        }

        typedef boost::function<std::map<FString, int64_t> (void) > StatFunction;

        /**
         * GetAllStats() used to get a map of all the stats in Derived
         * @retrun map of all the stats
         */
        virtual std::map<FString, int64_t> GetAllStats(void) {
            std::map<FString, int64_t> result;

            std::map<FString, int64_t> localValues =
                mStatVariables.template GetAllLocals<int64_t>(this);
            result.insert(localValues.begin(), localValues.end());

            foreach (const StatFunction fn, mStatFunctions)
            {
                localValues = fn();
                result.insert(localValues.begin(), localValues.end());
            }

            typedef std::pair<FString, ChildStatCall> ChildStatCallPair;
            foreach (const ChildStatCallPair &pair, mChildStats)
            {
                std::map<FString, int64_t> childResults = pair.second();
                typedef std::pair<FString, int64_t> StatResult;
                foreach (const StatResult& childResult, childResults)
                {
                    result.insert(
                        make_pair(
                            FString(
                                FStringFC(),
                                "%s.%s",
                                pair.first.c_str(),
                                childResult.first.c_str()),
                            childResult.second
                            )
                        );
                }
            }

            return result;
        }

        /**
         * GetStat() used to get the stat identified by "name"
         * @param name the name of the stat
         * @return the value of the stat
         */
        virtual int64_t GetStat(const Forte::FString &name) {

            if (mStatVariables.LocalExists(name))
            {
                return mStatVariables.template GetLocal<int64_t>(this, name);
            }
            else
            {
                std::vector<FString> components;
                name.Tokenize(".", components, 2);

                if (components.size() == 2 &&
                    mChildStats.find(components[0]) != mChildStats.end())
                {
                    return (mChildStats[components[0]]())[components[1]];
                }
            }

            boost::throw_exception(EFailedToFindStat(name));
        }

    protected:
        /**
         * includeStatsFromChild() If Derived class has other child objects which
         * also inherit from EnableStats (or BaseEnableStats) then we can use this
         * function to make the child objects stats accessible from Derived.
         * @param child the child object ptr
         * @param the name which will be prepended to the child object's stats names
         */
        virtual void includeStatsFromChild(
            const boost::shared_ptr<BaseEnableStats> &child,
            const Forte::FString &childStatObjName) {

            // first check if we can get a shared pointer
            // to the parent
            boost::shared_ptr<BaseEnableStats> self;
            try
            {
                self = boost::static_pointer_cast<Derived>(
                    static_cast<Derived*>(this)->shared_from_this());
            }
            catch (...)
            {
                // Failed to get the shared pointer for this, don't register
                // the parent with the child, or we may have trouble during
                // destruction time.
            }

            if (child)
            {
                registerChild(
                    childStatObjName,
                    WeakFunctionBinder(&BaseEnableStats::GetAllStats,
                                       child)
                    );

            }

            if (self)
            {
                child->registerParent(
                    boost::bind(
                        WeakFunctionBinder(
                            &BaseEnableStats::removeChild,
                            self),
                        childStatObjName));
            }
        }


        /**
         * registerStatVariable() Use this function to register a pointer to a local member
         * variable in Derived.
         * @param name the string identifier for the stat variable
         * @param var the pointer to the member variable in Derived
         */
        template <int index, typename VariableType>
        void registerStatVariable(
            const Forte::FString& name,
            VariableType (Derived:: * const var)) {

            mStatVariables.template AssignLocal<index>(name, var);
        }

        void registerStatFunction(StatFunction fn) {
            mStatFunctions.push_back(fn);
        }

    private:
        typedef boost::function<std::map<FString, int64_t> (void) > ChildStatCall;

        virtual void registerParent(
            boost::function<void()> parentUnregisterFn) {

            mParentUnregisterFn = parentUnregisterFn;
        }

        virtual void registerChild(
            const Forte::FString &childStatObjName,
            ChildStatCall call) {

            if (mChildStats.find(childStatObjName) == mChildStats.end())
            {
                mChildStats[childStatObjName] = call;
            }
        }

        virtual void removeChild(
            const Forte::FString &childStatObjName) {

            if (mChildStats.find(childStatObjName) != mChildStats.end())
            {
                mChildStats.erase(childStatObjName);
            }
        }

        virtual void removeChildFromParent() {

            if (mParentUnregisterFn) {
                try
                {
                    mParentUnregisterFn();
                }
                catch (EObjectCouldNotBeLocked &e)
                {
                    hlog(HLOG_DEBUG2, "Parent seems to be deleted!");
                }
                mParentUnregisterFn = boost::function<void()>();
            }
        }

    private:
        LocalsType mStatVariables;

        std::vector<StatFunction> mStatFunctions;

        std::map<FString, ChildStatCall> mChildStats;

        boost::function<void ()> mParentUnregisterFn;
    };

    //TODO: put this elsewhere
    class StatItem
    {
    public:
        StatItem()
            : mCount(0)
            {
            }

        StatItem(const StatItem& p) {
            Forte::AutoUnlockMutex lock(mMutex);
            mCount = p.mCount;
        }

        StatItem& operator++ () {
            Forte::AutoUnlockMutex lock(mMutex);
            ++mCount;
            return *this;
        }

        StatItem operator++ (int) {
            StatItem result(*this);
            ++(*this);
            return result;
        }

        StatItem& operator-- () {
            Forte::AutoUnlockMutex lock(mMutex);
            --mCount;
            return *this;
        }

        StatItem operator-- (int) {
            StatItem result(*this);
            --(*this);
            return result;
        }

        StatItem& operator=(const StatItem& rhs)
        {
            if(this != &rhs)
            {
                const int64_t count(rhs);

                {
                    Forte::AutoUnlockMutex lock(mMutex);
                    mCount = count;
                }
            }

            return *this;
        }

        StatItem operator-(const StatItem& rhs) const
        {
            StatItem result(*this);
            const int64_t count(rhs);
            Forte::AutoUnlockMutex lock(mMutex);
            result.mCount -= count;
            return result;
        }

        StatItem operator+(const StatItem& rhs) const
        {
            StatItem result(*this);
            const int64_t count(rhs);
            Forte::AutoUnlockMutex lock(mMutex);
            result.mCount += count;
            return result;
        }

        operator int64_t () const {
            Forte::AutoUnlockMutex lock(mMutex);
            return mCount;
        }

    protected:
        mutable Forte::Mutex mMutex;
        int64_t mCount;
    };

}
#endif
