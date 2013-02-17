#ifndef __Forte_WeakFunctionBinder_h__
#define __Forte_WeakFunctionBinder_h__

#include "Exception.h"

EXCEPTION_CLASS(EWeakFunctionBinder);
EXCEPTION_SUBCLASS2(EWeakFunctionBinder, EObjectCouldNotBeLocked,
                    "The object this function is bound to could not be locked");

namespace Forte
{
    template <typename RType, typename T>
        class WeakFunctionBinder0Args
    {
    public:
        typedef RType result_type;
        typedef RType (T::*MemberFunc)();

    WeakFunctionBinder0Args(
        MemberFunc func,
        const boost::weak_ptr<T> &obj) :
        mMemberFunc (func),
            mCallbackObj (obj) {}

        ~WeakFunctionBinder0Args() {}

        result_type operator()()
        {
            boost::shared_ptr<T> obj = mCallbackObj.lock();
            if (obj)
            {
                return ((*obj).*mMemberFunc)();
            }

            boost::throw_exception(EObjectCouldNotBeLocked());
        }

    private:
        MemberFunc mMemberFunc;
        boost::weak_ptr<T> mCallbackObj;
    };

    template <typename RType, typename T, typename Arg1>
        class WeakFunctionBinder1Args
    {
    public:
        typedef RType result_type;
        typedef RType (T::*MemberFunc)(Arg1);

    WeakFunctionBinder1Args(
        MemberFunc func,
        const boost::weak_ptr<T> &obj) :
        mMemberFunc (func),
            mCallbackObj (obj) {}

        ~WeakFunctionBinder1Args() {}

        result_type operator()(Arg1 arg1)
        {
            boost::shared_ptr<T> obj = mCallbackObj.lock();
            if (obj)
            {
                return ((*obj).*mMemberFunc)(arg1);
            }

            boost::throw_exception(EObjectCouldNotBeLocked());
        }

    private:
        MemberFunc mMemberFunc;
        boost::weak_ptr<T> mCallbackObj;
    };

    template <typename RType, typename T, typename Arg1, typename Arg2>
        class WeakFunctionBinder2Args
    {
    public:
        typedef RType result_type;
        typedef RType (T::*MemberFunc)(Arg1, Arg2);

    WeakFunctionBinder2Args(
        MemberFunc func,
        const boost::weak_ptr<T> &obj) :
        mMemberFunc (func),
            mCallbackObj (obj) {}

        ~WeakFunctionBinder2Args() {}

        result_type operator()(Arg1 arg1, Arg2 arg2)
        {
            boost::shared_ptr<T> obj = mCallbackObj.lock();
            if (obj)
            {
                return ((*obj).*mMemberFunc)(arg1, arg2);
            }

            boost::throw_exception(EObjectCouldNotBeLocked());
        }

    private:
        MemberFunc mMemberFunc;
        boost::weak_ptr<T> mCallbackObj;
    };

    template <typename RType, typename T, typename Arg1, typename Arg2, typename Arg3>
        class WeakFunctionBinder3Args
    {
    public:
        typedef RType result_type;
        typedef RType (T::*MemberFunc)(Arg1, Arg2, Arg3);

    WeakFunctionBinder3Args(
        MemberFunc func,
        const boost::weak_ptr<T> &obj) :
        mMemberFunc (func),
            mCallbackObj (obj) {}

        ~WeakFunctionBinder3Args() {}

        result_type operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3)
        {
            boost::shared_ptr<T> obj = mCallbackObj.lock();
            if (obj)
            {
                return ((*obj).*mMemberFunc)(arg1, arg2, arg3);
            }

            boost::throw_exception(EObjectCouldNotBeLocked());
        }

    private:
        MemberFunc mMemberFunc;
        boost::weak_ptr<T> mCallbackObj;
    };

    template <typename RType, typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        class WeakFunctionBinder4Args
    {
    public:
        typedef RType result_type;
        typedef RType (T::*MemberFunc)(Arg1, Arg2, Arg3, Arg4);

    WeakFunctionBinder4Args(
        MemberFunc func,
        const boost::weak_ptr<T> &obj) :
        mMemberFunc (func),
            mCallbackObj (obj) {}

        ~WeakFunctionBinder4Args() {}

        result_type operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
        {
            boost::shared_ptr<T> obj = mCallbackObj.lock();
            if (obj)
            {
                return ((*obj).*mMemberFunc)(arg1, arg2, arg3, arg4);
            }

            boost::throw_exception(EObjectCouldNotBeLocked());
        }

    private:
        MemberFunc mMemberFunc;
        boost::weak_ptr<T> mCallbackObj;
    };

    /* I wish i had variadic templates */
    template<typename RType, typename T>
        WeakFunctionBinder0Args<RType, T> WeakFunctionBinder(
            RType (T::*p)(),
            boost::shared_ptr<T> obj)
    {
        return WeakFunctionBinder0Args<RType, T>(p, obj);
    }

    template<typename RType, typename T, typename Arg1>
        WeakFunctionBinder1Args<RType, T, Arg1> WeakFunctionBinder(
            RType (T::*p)(Arg1),
            boost::shared_ptr<T> obj)
    {
        return WeakFunctionBinder1Args<RType, T, Arg1>(p, obj);
    }

    template<typename RType, typename T, typename Arg1, typename Arg2>
        WeakFunctionBinder2Args<RType, T, Arg1, Arg2> WeakFunctionBinder(
            RType (T::*p)(Arg1, Arg2),
            boost::shared_ptr<T> obj)
    {
        return WeakFunctionBinder2Args<RType, T, Arg1, Arg2>(p, obj);
    }

    template<typename RType, typename T, typename Arg1, typename Arg2, typename Arg3>
        WeakFunctionBinder3Args<RType, T, Arg1, Arg2, Arg3> WeakFunctionBinder(
            RType (T::*p)(Arg1, Arg2, Arg3),
            boost::shared_ptr<T> obj)
    {
        return WeakFunctionBinder3Args<RType, T, Arg1, Arg2, Arg3>(p, obj);
    }

    template<typename RType, typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        WeakFunctionBinder4Args<RType, T, Arg1, Arg2, Arg3, Arg4> WeakFunctionBinder(
            RType (T::*p)(Arg1, Arg2, Arg3, Arg4),
            boost::shared_ptr<T> obj)
    {
        return WeakFunctionBinder4Args<RType, T, Arg1, Arg2, Arg3, Arg4>(p, obj);
    }
};

#endif
