#ifndef __Forte_AsyncInvocation_h__
#define __Forte_AsyncInvocation_h__

#include "Event.h"
#include "Future.h"
#include <boost/function.hpp>

namespace Forte
{
    class AsyncInvocation : public Forte::Event
    {
    public:
        virtual void Execute(void) = 0;
        virtual ~AsyncInvocation() {};
    };

    template<typename RetvalType>
    class ConcreteInvocation : public Forte::AsyncInvocation
    {
    public:
        typedef boost::function<RetvalType(void)> CallbackT;
        ConcreteInvocation(const boost::shared_ptr<Forte::Future<RetvalType> > &future,
                           CallbackT callback) :
            mFuture(future), mCallback(callback) {};

        virtual void Execute(void) {
            try
            {
                mFuture->SetResult(mCallback());
                mFuture->SetException(boost::exception_ptr());
            }
            catch (...)
            {
                mFuture->SetException(boost::current_exception());
            }
        }

        boost::shared_ptr<Forte::Future<RetvalType> > mFuture;
        CallbackT mCallback;
    };

    template<>
    class ConcreteInvocation<void> : public Forte::AsyncInvocation
    {
    public:
        typedef boost::function<void(void)> CallbackT;
        ConcreteInvocation(const boost::shared_ptr<Forte::Future<void> > &future,
                           CallbackT callback) :
            mFuture(future), mCallback(callback) {};

        virtual void Execute(void) {
            try
            {
                mCallback();
                mFuture->SetException(boost::exception_ptr());
            }
            catch (...)
            {
                mFuture->SetException(boost::current_exception());
            }
        }
        boost::shared_ptr<Forte::Future<void> > mFuture;
        CallbackT mCallback;
    };
};

#endif

