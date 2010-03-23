#include "Forte.h"
#define BOOST_TEST_MODULE "Context Unit Tests"
#include <boost/test/unit_test.hpp>

#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits.hpp>
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <boost/lambda/loops.hpp>
#include <boost/lambda/switch.hpp>

namespace bll = boost::lambda;

struct ContextFixture {
    Forte::Context c;
};

class TestClass : public Forte::Object {
public:
    TestClass() { ++mCount; }
    virtual ~TestClass() { --mCount; }
    int getCount(void) { return mCount; }
    virtual bool isDerived(void) { return false; }
    static int mCount;
};

class TestClassDerived : public TestClass {
public:
    TestClassDerived() {};
    virtual ~TestClassDerived() {};
    virtual bool isDerived(void) { return true; }
};
int TestClass::mCount = 0;

BOOST_FIXTURE_TEST_SUITE( context, ContextFixture );
BOOST_AUTO_TEST_CASE ( context_test1 )
{
    CLogManager logMgr;
    logMgr.BeginLogging();
    logMgr.SetGlobalLogMask(HLOG_ALL);

    // verify invalid keys throws
    BOOST_CHECK_THROW(c.Get<CLogManager>("forte.logManager"), EInvalidKey);
    
    c.Set("forte.config", new CServiceConfig());

    BOOST_CHECK_THROW(c.Get<CServiceConfig>("forte.config.fail"), EInvalidKey);

    // verify type mismatch throws
    BOOST_CHECK_THROW(c.Get<CLogManager>("forte.config"), EContextTypeMismatch);

    {
        shared_ptr<CServiceConfig> cfg = (c.Get<CServiceConfig>("forte.config"));
        cfg->Set("key", "value");
    }

    // verify objects are destroyed and reference counts work correctly
    BOOST_CHECK(TestClass::mCount == 0);
    c.Set("testobject", new TestClass());
    BOOST_CHECK(TestClass::mCount == 1);
    c.Remove("testobject");
    BOOST_CHECK(TestClass::mCount == 0);
    BOOST_CHECK_THROW(c.Get<TestClass>("testobject"), EInvalidKey);

    c.Set("testobject", new TestClass());
    BOOST_CHECK(TestClass::mCount == 1);
    {
        Forte::ObjectPtr oPtr(c.Get("testobject"));
        shared_ptr<TestClass> tcPtr(dynamic_pointer_cast<TestClass>(oPtr));
        BOOST_CHECK( tcPtr->getCount() == 1 );
        BOOST_CHECK( tcPtr->isDerived() == false );
        c.Remove("testobject");
        BOOST_CHECK( TestClass::mCount == 1 );
    }
    BOOST_CHECK(TestClass::mCount == 0);

    // test object replacement and up-casting:
    c.Set("testobject", new TestClass());
    BOOST_CHECK( TestClass::mCount == 1 );
    c.Set("testobject", new TestClassDerived());
    BOOST_CHECK( TestClass::mCount == 1 );
    {
        Forte::ObjectPtr oPtr(c.Get("testobject"));
        shared_ptr<TestClass> tcPtr(dynamic_pointer_cast<TestClass>(oPtr));
        BOOST_CHECK( tcPtr->getCount() == 1 );
        BOOST_CHECK( tcPtr->isDerived() == true );
        c.Remove("testobject");
        BOOST_CHECK( TestClass::mCount == 1 );
    }
    BOOST_CHECK(TestClass::mCount == 0);

//    boost::bind( boost::mem_fn(&Forte::Context::GetRef<TestClass>), _1, "testobject");
    c.Set("testobject", new TestClassDerived());
    BOOST_CHECK( TestClass::mCount == 1 );
    bll::bind( &Context::Get<TestClass>, bll::_1, "testobject");  // functor to retrieve object from context

//  This works:
    boost::function<shared_ptr<TestClass>(const Context&)> f = 
        bll::bind(&Context::Get<TestClass>, bll::_1, "testobject");
    f(c);
    boost::function<int(TestClass&)> g = 
        bll::bind(&TestClass::getCount, bll::_1);

    // Not sure how to bind this such that 'c' is the parameter:
    g(*(f(c)));
    
//    boost::function<bool(Context&)> h =
//    bll::bind(f, bll::_1);
//    (bll::unlambda(g)(*(bll::unlambda(f)(bll::_1))) == 1);
//    boost::bind(bll::unlambda(g),


//    h(c);

    std::set<FString> validStrs;
    validStrs.insert("validStr");
    c.Set("checkedvalue", new CheckedStringEnum(validStrs));
    c.Get<CheckedStringEnum>("checkedvalue")->Set("validStr");
    
//        bll::bind(bll::bind(&Context::GetRef<TestClass>, bll::_1, "testobject"),bll::_1);
//    b(c);
        
//    boost::function<bool(Context&)> b = 
    bll::bind(
        &Context::Get<CheckedStringEnum>, bll::_1, "checkedvalue");
// == "validStr");
    
    
//    int x = 
//        bll::bind<Forte::Context>(bll::bind( &Context::GetRef<TestClass>, bll::_1 )("testobject").getCount())(c);

}
BOOST_AUTO_TEST_SUITE_END();

