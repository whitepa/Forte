#ifndef __Callback_h
#define __Callback_h

#include "Exception.h"

#ifndef FORTE_NO_BOOST
#include <boost/function.hpp>
#endif

EXCEPTION_SUBCLASS(CForteException, CForteCallbackException);

class CCallback
{
public:
    CCallback() {};
    virtual ~CCallback() {};
    virtual void operator()(void *arg = NULL) const = 0;
    virtual void execute(void *arg = NULL) const = 0;
};

class CStaticCallback : public CCallback
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
    void execute(void *arg) const
    {
        (*method)();
    }
    Method method;
};

class CStaticParamCallback : public CCallback
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
        if (!method) throw CForteCallbackException("Invalid Callback");
        (*method)(arg);
    }
    void execute(void *arg) const
    {
        if (!method) throw CForteCallbackException("Invalid Callback");
        (*method)(arg);
    }
    Method method;
};

template < class Class >
class CInstanceCallback : public CCallback
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
    void execute(void *arg) const
    {
        operator()();
    };
private:
    Class*  class_instance;
    Method  method;
};

template < class Class >
class CInstanceParamCallback : public CCallback
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
    void execute(void *arg) const
    {
        operator()(arg);
    };
private:
    Class*  class_instance;
    Method  method;
};

#ifndef FORTE_NO_BOOST
class CFunctorCallback : public CCallback
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
    void execute(void *arg) const
    {
        mFunc();
    };
    boost::function<void()> mFunc;
};
#endif
#endif
