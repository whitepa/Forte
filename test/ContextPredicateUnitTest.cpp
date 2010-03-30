#include "Forte.h"
#define BOOST_TEST_MODULE "ContextPredicate Unit Tests"
#include <boost/test/unit_test.hpp>

static const char *validStrArray[] = {
    "One",
    "Two",
    "Three",
    NULL
};

struct ContextPredicateFixture {
    ContextPredicateFixture() { BOOST_TEST_MESSAGE("setup fixture"); }

    Forte::Context c1;
    Forte::Context c2;
};

BOOST_FIXTURE_TEST_SUITE( s, ContextPredicateFixture );
BOOST_AUTO_TEST_CASE ( s_test1 )
{
    LogManager logMgr;
    logMgr.BeginLogging();
    logMgr.SetGlobalLogMask(HLOG_ALL);

    // test string equality
    c1.Set("string", ObjectPtr(new CheckedStringEnum(validStrArray)));
    c2.Set("string", ObjectPtr(new CheckedStringEnum(validStrArray)));
    c1.Get<CheckedStringEnum>("string")->Set("One");
    c2.Get<CheckedStringEnum>("string")->Set("Two");
    Forte::ContextPredicate cp1(Forte::ContextPredicate::CHECKED_STRING_EQUALS,
                               "string", "One");    
    BOOST_CHECK( cp1.Evaluate(c1) == true );
    BOOST_CHECK( cp1.Evaluate(c2) == false );

    // test int equality
    c1.Set("int1", ObjectPtr(new CheckedInt32()));
    c2.Set("int1", ObjectPtr(new CheckedInt32()));
    c1.Get<CheckedInt32>("int1")->Set(1);
    c2.Get<CheckedInt32>("int1")->Set(2);
    Forte::ContextPredicate cp2(Forte::ContextPredicate::CHECKED_INT_EQUALS,
                                "int1", 1);
    BOOST_CHECK( cp2.Evaluate(c1) == true );
    BOOST_CHECK( cp2.Evaluate(c2) == false );

    // test int GTE
    Forte::ContextPredicate cp3(Forte::ContextPredicate::CHECKED_INT_GTE,
                                "int1", 2);
    Forte::ContextPredicate cp4(Forte::ContextPredicate::CHECKED_INT_GTE,
                                "int1", 0);
    BOOST_CHECK( cp3.Evaluate(c1) == false );
    BOOST_CHECK( cp3.Evaluate(c2) == true );
    BOOST_CHECK( cp4.Evaluate(c1) == true );
    BOOST_CHECK( cp4.Evaluate(c2) == true );

    // test exception from checked int (uninitialized)
    c1.Set("int2", ObjectPtr(new CheckedInt32()));
    Forte::ContextPredicate cp5(Forte::ContextPredicate::CHECKED_INT_GTE,
                                "int2", 0);
    BOOST_CHECK_THROW( cp5.Evaluate(c1), ECheckedValueUninitialized );
    c1.Get<CheckedInt32>("int2")->Set(0);
    BOOST_CHECK( cp5.Evaluate(c1) == true );

    // test invalid predicate exception
    Forte::ContextPredicate cp6(Forte::ContextPredicate::NOT, cp4, cp5);
    BOOST_CHECK_THROW( cp6.Evaluate(c1), EContextPredicateInvalid );

    // test AND
    BOOST_CHECK(
        Forte::ContextPredicate(
            Forte::ContextPredicate::AND, cp1, cp2, cp4)
        .Evaluate(c1) == true);
    BOOST_CHECK(
        Forte::ContextPredicate(
            Forte::ContextPredicate::AND, cp1, cp2, cp4)
        .Evaluate(c2) == false);

    // test OR
    BOOST_CHECK(
        Forte::ContextPredicate(
            Forte::ContextPredicate::OR, cp1, cp2, cp4)
        .Evaluate(c1) == true);
    BOOST_CHECK(
        Forte::ContextPredicate(
            Forte::ContextPredicate::OR, cp1, cp2)
        .Evaluate(c2) == false);

    // test NOT
    BOOST_CHECK(
        Forte::ContextPredicate(
            Forte::ContextPredicate::NOT, cp1)
        .Evaluate(c1) == false);
    BOOST_CHECK(
        Forte::ContextPredicate(
            Forte::ContextPredicate::NOT, cp1)
        .Evaluate(c2) == true);
}
BOOST_AUTO_TEST_SUITE_END();

