#include "Forte.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace Forte;

static const char *validStrArray[] = {
    "One",
    "Two",
    "Three",
    NULL
};

class ContextPredicateTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        signal(SIGPIPE, SIG_IGN);
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");

    };

    Forte::Context c1;
    Forte::Context c2;

    static void TearDownTestCase() {

    };

};

TEST_F (ContextPredicateTest, s_test1)
{
    // test string equality
    c1.Set("string", ObjectPtr(new CheckedStringEnum(validStrArray)));
    c2.Set("string", ObjectPtr(new CheckedStringEnum(validStrArray)));
    c1.Get<CheckedStringEnum>("string")->Set("One");
    c2.Get<CheckedStringEnum>("string")->Set("Two");
    Forte::ContextPredicate cp1(Forte::ContextPredicate::CHECKED_STRING_EQUALS,
                               "string", "One");    

    EXPECT_TRUE( cp1.Evaluate(c1) );
    EXPECT_FALSE( cp1.Evaluate(c2) );

    // test int equality
    c1.Set("int1", ObjectPtr(new CheckedInt32()));
    c2.Set("int1", ObjectPtr(new CheckedInt32()));
    c1.Get<CheckedInt32>("int1")->Set(1);
    c2.Get<CheckedInt32>("int1")->Set(2);
    Forte::ContextPredicate cp2(Forte::ContextPredicate::CHECKED_INT_EQUALS,
                                "int1", 1);
    EXPECT_TRUE( cp2.Evaluate(c1) );
    EXPECT_FALSE( cp2.EAvaluate(c2) );

    // test int GTE
    Forte::ContextPredicate cp3(Forte::ContextPredicate::CHECKED_INT_GTE,
                                "int1", 2);
    Forte::ContextPredicate cp4(Forte::ContextPredicate::CHECKED_INT_GTE,
                                "int1", 0);
    EXPECT_FALSE( cp3.Evaluate(c1) );
    EXPECT_TRUE( cp3.Evaluate(c2) );
    EXPECT_TRUE( cp4.Evaluate(c1) );
    EXPECT_TRUE( cp4.Evaluate(c2) );

    // test exception from checked int (uninitialized)
    c1.Set("int2", ObjectPtr(new CheckedInt32()));
    Forte::ContextPredicate cp5(Forte::ContextPredicate::CHECKED_INT_GTE,
                                "int2", 0);
    EXPECT_THROW( cp5.Evaluate(c1), ECheckedValueUninitialized );
    c1.Get<CheckedInt32>("int2")->Set(0);
    EXPECT_TRUE( cp5.Evaluate(c1) );

    // test invalid predicate exception
    Forte::ContextPredicate cp6(Forte::ContextPredicate::NOT, cp4, cp5);
    EXPECT_THROW( cp6.Evaluate(c1), EContextPredicateInvalid );

    // test AND
    EXPECT_TRUE(
        Forte::ContextPredicate(
            Forte::ContextPredicate::AND, cp1, cp2, cp4)
        .Evaluate(c1) == true);
    EXPECT_FALSE(
        Forte::ContextPredicate(
            Forte::ContextPredicate::AND, cp1, cp2, cp4)
        .Evaluate(c2) == false);

    // test OR
    EXPECT_TRUE(
        Forte::ContextPredicate(
            Forte::ContextPredicate::OR, cp1, cp2, cp4)
        .Evaluate(c1) == true);
    EXPECT_FALSE(
        Forte::ContextPredicate(
            Forte::ContextPredicate::OR, cp1, cp2)
        .Evaluate(c2) == false);

    // test NOT
    EXPECT_FALSE(
        Forte::ContextPredicate(
            Forte::ContextPredicate::NOT, cp1)
        .Evaluate(c1) == false);
    EXPECT_TRUE(
        Forte::ContextPredicate(
            Forte::ContextPredicate::NOT, cp1)
        .Evaluate(c2) == true);
}

