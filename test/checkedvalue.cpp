#include "Forte.h"
#define BOOST_TEST_MODULE "Checked Value Unit Tests"
#include <boost/test/unit_test.hpp>

struct Int32Fixture {
    Int32Fixture()  { BOOST_TEST_MESSAGE( "setup fixture" );    }
    ~Int32Fixture() { BOOST_TEST_MESSAGE( "teardown fixture" ); }

    Forte::CheckedInt32 a;
};

static const char *validStrArray[] = {
    "One",
    "Two",
    "Three",
    NULL
};

struct StringEnumFixture {
    StringEnumFixture() : a(validStrArray) { BOOST_TEST_MESSAGE("setup fixture"); }
    Forte::CheckedStringEnum a;
};

BOOST_FIXTURE_TEST_SUITE( checked_values, Int32Fixture );
BOOST_AUTO_TEST_CASE ( int_test )
{
    CLogManager logMgr;
    logMgr.BeginLogging();
    logMgr.SetGlobalLogMask(HLOG_ALL);

    int flags = a.GetFlags();
    BOOST_CHECK( (flags & Forte::CheckedValue::UNINITIALIZED) != 0);
    BOOST_CHECK( (flags & Forte::CheckedValue::EXPIRED) == 0);
    BOOST_CHECK( (flags & Forte::CheckedValue::INVALID) == 0);
    BOOST_CHECK( (flags & Forte::CheckedValue::POLL_FAILED) == 0);

    BOOST_CHECK_THROW ( a.Get(), ECheckedValueUninitialized );

    a.Set(0);
    flags = a.GetFlags();
    BOOST_CHECK( (flags & Forte::CheckedValue::UNINITIALIZED) == 0);
    BOOST_CHECK( a.Get() == 0 );

    a.Set(1);
    BOOST_CHECK( a.Get() == 1 );
}
BOOST_AUTO_TEST_SUITE_END();

BOOST_FIXTURE_TEST_SUITE( checked_strings, StringEnumFixture );
BOOST_AUTO_TEST_CASE ( int_test )
{
    CLogManager logMgr;
    logMgr.BeginLogging();
    logMgr.SetGlobalLogMask(HLOG_ALL);
    
    // uninitialized get should throw
    BOOST_CHECK_THROW(a.Get(), ECheckedValueUninitialized);

    a.Set("One");
    BOOST_CHECK(a.Get() == "One");

    a.Set("Two");
    BOOST_CHECK(a.Get() == "Two");

    // invalid
    a.Set("Four");
    BOOST_CHECK(a.Get() == "Four");
    
    a.SetMode(Forte::CheckedValue::INVALID_GET_THROWS | 
              Forte::CheckedValue::UNINITIALIZED_GET_THROWS);
    BOOST_CHECK_THROW(a.Get(), ECheckedValueInvalid);

    // should not throw:
    a.Set("Five");
    BOOST_CHECK_THROW(a.Get(), ECheckedValueInvalid);
    a.SetMode(Forte::CheckedValue::INVALID_GET_THROWS | 
              Forte::CheckedValue::INVALID_SET_THROWS | 
              Forte::CheckedValue::UNINITIALIZED_GET_THROWS);
    BOOST_CHECK_THROW(a.Get(), ECheckedValueInvalid);
    BOOST_CHECK_THROW(a.Set("Six"), ECheckedValueInvalid);

    // this will throw, uncomment to test the test framework
    //a.Set("Seven");
}
BOOST_AUTO_TEST_SUITE_END();
