#ifndef __forte__EnableStats_h__
#define __forte__EnableStats_h__

#include <map>
#include "FString.h"
#include "Foreach.h"
#include "Exception.h"
#include "AutoMutex.h"
#include "LogManager.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace Forte
{

    EXCEPTION_CLASS(EEnableStats);
    EXCEPTION_SUBCLASS2(EEnableStats, EFailedToFindStat,
                        "Failed to find stat");

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


        virtual void AddChildTo(
            BaseEnableStats *parent,
            const Forte::FString &childStatObjName) {
            // do nothing by default
        }

    protected:
        typedef boost::function<std::map<FString, int64_t> (void) > ChildStatCall;

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


    template <typename Derived, typename LocalsType>
        class EnableStats : public virtual BaseEnableStats
    {
    public:
        typedef boost::function<std::map<FString, int64_t> (void) > ChildStatCall;

        EnableStats() {}

        virtual ~EnableStats() {
            removeChildFromParent();
        }

        virtual std::map<FString, int64_t> GetAllStats(void) {
            std::map<FString, int64_t> result;

            std::map<FString, int64_t> localValues =
                mStatVariables.template GetAllLocals<int64_t>(this);
            result.insert(localValues.begin(), localValues.end());

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

        virtual void AddChildTo(
                BaseEnableStats *parent,
                const Forte::FString &childStatObjName) {

            parent->registerChild(
                childStatObjName,
                boost::bind(&EnableStats<Derived, LocalsType>::GetAllStats, this)
                );

            mParentUnregisterFn = boost::bind(
                &BaseEnableStats::removeChild,
                parent, childStatObjName);
        }

    protected:
        template <int index, typename VariableType>
        void registerStatVariable(
            const Forte::FString& name,
            VariableType (Derived:: * const var)) {

            mStatVariables.template AssignLocal<index>(name, var);
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
                mParentUnregisterFn();
                mParentUnregisterFn = boost::function<void()>();
            }
        }

    protected:
        LocalsType mStatVariables;
        std::map<FString, ChildStatCall> mChildStats;

        boost::function<void ()> mParentUnregisterFn;
    };

}
#endif
