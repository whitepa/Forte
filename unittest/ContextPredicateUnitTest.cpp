#include "Forte.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace Forte;

LogManager logManager;

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
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
    };

    Forte::Context c1;
    Forte::Context c2;

    static void TearDownTestCase() {

    };

    static ContextPredicateFunctionResult PredicateFunctionSet(
            const Forte::Context &c)
    {
        return make_tuple(true, MonotonicClock().GetTime());
    }

    static ContextPredicateFunctionResult PredicateFunctionClear(
            const Forte::Context &c)
    {
        return make_tuple(false, MonotonicClock().GetTime());
    }
};

TEST_F (ContextPredicateTest, s_test1)
{
    // test string equality
    c1.Set("string", ObjectPtr(new CheckedStringEnum(validStrArray)));
    c2.Set("string", ObjectPtr(new CheckedStringEnum(validStrArray)));
    c1.Get<CheckedStringEnum>("string")->Set("One");
    c2.Get<CheckedStringEnum>("string")->Set("Two");
    Forte::ContextPredicatePtr cp1 = make_shared<ContextPredicate>(Forte::ContextPredicate::CHECKED_STRING_EQUALS,
                               "string", "One");    

    EXPECT_TRUE( cp1->Evaluate(c1) );
    EXPECT_FALSE( cp1->Evaluate(c2) );

    // test int equality
    c1.Set("int1", ObjectPtr(new CheckedInt32()));
    c2.Set("int1", ObjectPtr(new CheckedInt32()));
    c1.Get<CheckedInt32>("int1")->Set(1);
    c2.Get<CheckedInt32>("int1")->Set(2);
    Forte::ContextPredicatePtr cp2 = make_shared<ContextPredicate>(Forte::ContextPredicate::CHECKED_INT_EQUALS,
                                "int1", 1);
    EXPECT_TRUE( cp2->Evaluate(c1) );
    EXPECT_FALSE( cp2->Evaluate(c2) );

    // test int GTE
    Forte::ContextPredicatePtr cp3 = make_shared<ContextPredicate>(Forte::ContextPredicate::CHECKED_INT_GTE,
                                "int1", 2);
    Forte::ContextPredicatePtr cp4 = make_shared<ContextPredicate>(Forte::ContextPredicate::CHECKED_INT_GTE,
                                "int1", 0);
    c1.Get<CheckedInt32>("int1")->Set(1);
    EXPECT_FALSE( cp3->Evaluate(c1) );
    EXPECT_TRUE( cp3->Evaluate(c2) );
    EXPECT_TRUE( cp4->Evaluate(c1) );
    EXPECT_TRUE( cp4->Evaluate(c2) );

    // test exception from checked int (uninitialized)
    c1.Set("int2", ObjectPtr(new CheckedInt32()));
    Forte::ContextPredicatePtr cp5 = make_shared<ContextPredicate>(Forte::ContextPredicate::CHECKED_INT_GTE,
                                "int2", 0);
    EXPECT_THROW( cp5->Evaluate(c1, Timespec::FromSeconds(-1)),
                  ECheckedValueUninitialized );

    c1.Get<CheckedInt32>("int2")->Set(0);
    EXPECT_TRUE( cp5->Evaluate(c1) );

    // test invalid predicate exception
    Forte::ContextPredicatePtr cp6 = make_shared<ContextPredicate>(
            Forte::ContextPredicate::NOT, cp4, cp5);
    EXPECT_THROW( cp6->Evaluate(c1), EContextPredicateInvalid );

    // test predicate function
    Forte::ContextPredicatePtr cp7 = make_shared<ContextPredicate>(
            Forte::ContextPredicate::PREDICATE_FUNCTION_SET,
                                boost::make_shared<Forte::ContextPredicateFunction>(
                                    ContextPredicateTest::PredicateFunctionSet));
    EXPECT_TRUE( cp7->Evaluate(c1) );

    Forte::ContextPredicatePtr cp8 = make_shared<ContextPredicate>(
            Forte::ContextPredicate::PREDICATE_FUNCTION_CLEAR,
                                boost::make_shared<Forte::ContextPredicateFunction>(
                                    ContextPredicateTest::PredicateFunctionClear));
    EXPECT_TRUE( cp8->Evaluate(c1) );

    // test AND
    EXPECT_TRUE(cp1->Evaluate(c1));
    EXPECT_TRUE(cp2->Evaluate(c1));
    EXPECT_TRUE(cp4->Evaluate(c1));
    EXPECT_TRUE(
        Forte::ContextPredicate(
            Forte::ContextPredicate::AND, cp1, cp2, cp4)
        .Evaluate(c1) == true);

    c2.Get<CheckedInt32>("int1")->Set(1);
    EXPECT_TRUE(cp2->Evaluate(c2));
    EXPECT_FALSE(
        Forte::ContextPredicate(
            Forte::ContextPredicate::AND, cp1, cp2, cp4)
        .Evaluate(c2));

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
    EXPECT_TRUE(cp1->Evaluate(c1));
    EXPECT_FALSE(
        Forte::ContextPredicate(
            Forte::ContextPredicate::NOT, cp1)
        .Evaluate(c1));
    EXPECT_TRUE(
        Forte::ContextPredicate(
            Forte::ContextPredicate::NOT, cp1)
        .Evaluate(c2));
}

