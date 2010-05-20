#define BOOST_TEST_MODULE "Context Unit Tests"

#include <boost/test/unit_test.hpp>
#include "LogManager.h"
#include "Context.h"
#include "FileSystem.h"
#include "ServiceConfig.h"
#include "ProcFileSystem.h"
#include "MockFileSystem.h"


using namespace boost;
using namespace boost::unit_test;
using namespace Forte;

LogManager logManager;

class TestClass : public Forte::Object {
public:
    TestClass() { ++sCount; }
    virtual ~TestClass() { --sCount; }
    int getCount(void) { return sCount; }
    virtual bool isDerived(void) { return false; }
    static int sCount;
};

class TestClassDerived : public TestClass {
public:
    TestClassDerived() {};
    virtual ~TestClassDerived() {};
    virtual bool isDerived(void) { return true; }
};
int TestClass::sCount = 0;



void contextGetObject(const Forte::Context &context)
{
    ObjectPtr ob;
    ob = context.Get("objecttest");
    ob = context.Get<Object>("objecttest");
}


void contextGetFileSystem(const Forte::Context &context)
{
    shared_ptr<FileSystem> fsptr;
    
    fsptr = context.Get<FileSystem>("filesystemtest");
    fsptr->GetCWD();
}

struct ContextFixture {
    ContextFixture() { mgr.BeginLogging(); }
    Forte::Context c;
    Forte::LogManager mgr;
};

BOOST_FIXTURE_TEST_SUITE( context, ContextFixture );

BOOST_AUTO_TEST_CASE(ContextGetInvalidKey)
{
    BOOST_CHECK_THROW(contextGetObject(c), EInvalidKey);
}

BOOST_AUTO_TEST_CASE(ContextSetGetObject)
{
    ObjectPtr ptr(new Object());
    c.Set("objecttest", ptr);
    contextGetObject(c);
}

BOOST_AUTO_TEST_CASE(ContextSetGetFileSystem)
{
    shared_ptr<FileSystem> fsptr(new FileSystem());
    c.Set("filesystemtest", fsptr);
    contextGetFileSystem(c);
}

BOOST_AUTO_TEST_CASE(ContextSetObjectGetFileSystem)
{
    ObjectPtr ptr(new Object());
    c.Set("filesystemtest", ptr);
    BOOST_CHECK_THROW(contextGetFileSystem(c), EContextTypeMismatch)
        }

BOOST_AUTO_TEST_CASE(UptimeMockContents)
{
    // setup
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/uptime", "30782.38 29768.69\n");

    c.Set("forte.FileSystem", fsptr);

    // construct
    ProcFileSystem procFileSystem(c);

    // test uptime
    ProcFileSystem::Uptime uptime;
    procFileSystem.UptimeRead(uptime);
    
    BOOST_CHECK_EQUAL(uptime.mSecondsUp, 30782.38);
    BOOST_CHECK_EQUAL(uptime.mSecondsIdle, 29768.69);
}



BOOST_AUTO_TEST_CASE(PointerCastPreservesReference)
{
    hlog(HLOG_INFO, "PointerCastPreservesReference");
    {
        shared_ptr<TestClass> ptr;
        BOOST_CHECK(TestClass::sCount == 0);
        {
            // this test case needs its own context, as it must be destroyed
            Context c;
            {
                shared_ptr<TestClassDerived> ptr2(new TestClassDerived());
                c.Set("testclass", ptr2);
                BOOST_CHECK(TestClass::sCount == 1);
            }
            BOOST_CHECK(TestClass::sCount == 1);
            ptr = c.Get<TestClass>("testclass");
            BOOST_CHECK(TestClass::sCount == 1);
            {
                shared_ptr<TestClass> ptr3(c.Get<TestClass>("testclass"));
                BOOST_CHECK(TestClass::sCount == 1);
                BOOST_CHECK(ptr3.use_count() == 3);
                BOOST_CHECK(ptr.use_count() == 3);
            }
            BOOST_CHECK(TestClass::sCount == 1);            
            BOOST_CHECK(ptr.use_count() == 2);            
        }
        // context is out of scope
        BOOST_CHECK(TestClass::sCount == 1);
        BOOST_CHECK(ptr.use_count() == 1);            
    }
    BOOST_CHECK(TestClass::sCount == 0);
}

BOOST_AUTO_TEST_CASE(MiscTests)
{
    hlog(HLOG_INFO, "MiscTests");
    // verify invalid keys throws
    BOOST_CHECK_THROW(c.Get<LogManager>("forte.logManager"), EInvalidKey);

    c.Set("forte.config", ObjectPtr(new ServiceConfig()));

    BOOST_CHECK_THROW(c.Get<ServiceConfig>("forte.config.fail"), EInvalidKey);

    // verify type mismatch throws
    BOOST_CHECK_THROW(c.Get<LogManager>("forte.config"), EContextTypeMismatch);

    {
        shared_ptr<ServiceConfig> cfg = (c.Get<ServiceConfig>("forte.config"));
        cfg->Set("key", "value");
    }

    // verify objects are destroyed and reference counts work correctly
    BOOST_CHECK(TestClass::sCount == 0);
    c.Set("testobject", ObjectPtr(new TestClass()));
    BOOST_CHECK(TestClass::sCount == 1);
    c.Remove("testobject");
    BOOST_CHECK(TestClass::sCount == 0);
    BOOST_CHECK_THROW(c.Get<TestClass>("testobject"), EInvalidKey);

    c.Set("testobject", ObjectPtr(new TestClass()));
    BOOST_CHECK(TestClass::sCount == 1);
    {
        Forte::ObjectPtr oPtr(c.Get("testobject"));
        shared_ptr<TestClass> tcPtr(dynamic_pointer_cast<TestClass>(oPtr));
        BOOST_CHECK( tcPtr->getCount() == 1 );
        BOOST_CHECK( tcPtr->isDerived() == false );
        c.Remove("testobject");
        BOOST_CHECK( TestClass::sCount == 1 );
    }
    BOOST_CHECK(TestClass::sCount == 0);

    // test object replacement and up-casting:
    c.Set("testobject", ObjectPtr(new TestClass()));
    BOOST_CHECK( TestClass::sCount == 1 );
    c.Set("testobject", ObjectPtr(new TestClassDerived()));
    BOOST_CHECK( TestClass::sCount == 1 );
    {
        Forte::ObjectPtr oPtr(c.Get("testobject"));
        shared_ptr<TestClass> tcPtr(dynamic_pointer_cast<TestClass>(oPtr));
        BOOST_CHECK( tcPtr->getCount() == 1 );
        BOOST_CHECK( tcPtr->isDerived() == true );
        c.Remove("testobject");
        BOOST_CHECK( TestClass::sCount == 1 );
    }
    BOOST_CHECK(TestClass::sCount == 0);
}
BOOST_AUTO_TEST_SUITE_END();
