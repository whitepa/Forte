#ifndef __Callback_h
#define __Callback_h

#include "Exception.h"

#ifndef FORTE_NO_BOOST
#include <boost/function.hpp>
#endif

/**
 * The Callback classes are DEPRECATED.  boost::bind is superior.
 */

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, ForteCallbackException);
    class Callback : public Object
    {
    public:
        Callback() {};
        virtual ~Callback() {};
        virtual void operator()(void *arg = NULL) const = 0;
        virtual void Execute(void *arg = NULL) const = 0;
    };

    class CStaticCallback : public Callback
    {
    public:
        typedef void (*Method)(void);
        CStaticCallback(Method _method)
            {
                method = _method;
            }
        void operator()(void *arg) const
            {
                (*method)();
            }
        void Execute(void *arg) const
            {
                (*method)();
            }
        Method method;
    };

    class CStaticParamCallback : public Callback
    {
    public:
        typedef void (*Method)(void *);
        CStaticParamCallback(Method _method)
            {
                method = _method;
            }
        CStaticParamCallback() :
            method(NULL) {}
        CStaticParamCallback & operator=(const CStaticParamCallback &other)
            {
                method = other.method;
                return *this;
            }
        void operator()(void *arg) const
            {
                if (!method) throw ForteCallbackException("Invalid Callback");
                (*method)(arg);
            }
        void Execute(void *arg) const
            {
                if (!method) throw ForteCallbackException("Invalid Callback");
                (*method)(arg);
            }
        Method method;
    };

    template < class Class >
    class CInstanceCallback : public Callback
    {
    public:
        typedef void (Class::*Method)(void);
        CInstanceCallback(Class* _class_instance, Method _method)
            {
                class_instance = _class_instance;
                method        = _method;
            };
        void operator()(void *arg) const
            {
                (class_instance->*method)();
            };
        void Execute(void *arg) const
            {
                operator()();
            };
    private:
        Class*  class_instance;
        Method  method;
    };

    template < class Class >
    class CInstanceParamCallback : public Callback
    {
    public:
        typedef void (Class::*Method)(void *);
        CInstanceParamCallback(Class* _class_instance, Method _method)
            {
                class_instance = _class_instance;
                method        = _method;
            };
        void operator()(void *arg) const
            {
                (class_instance->*method)(arg);
            };
        void Execute(void *arg) const
            {
                operator()(arg);
            };
    private:
        Class*  class_instance;
        Method  method;
    };

#ifndef FORTE_NO_BOOST
    class CFunctorCallback : public Callback
    {
    public:
        CFunctorCallback(boost::function<void()> func)
            {
                mFunc = func;
            };
        void operator()(void *arg) const
            {
                mFunc();
            };
        void Execute(void *arg) const
            {
                mFunc();
            };
        boost::function<void()> mFunc;
    };
#endif
};
#endif
