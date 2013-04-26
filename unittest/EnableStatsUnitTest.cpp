#include <gtest/gtest.h>
#include "EnableStats.h"
#include "LogManager.h"
#include "Locals.h"
#include <boost/mpl/size.hpp>

using namespace Forte;
using namespace boost;

LogManager logManager;

class EnableStatsUnitTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.BeginLogging("//stdout");
        logManager.SetLogMask("//stdout", HLOG_ALL);
        hlog(HLOG_DEBUG, "Starting test...");
    }

    static void TearDownTestCase() {
        logManager.EndLogging("//stdout");
    }

    static void PrintStats(
        std::map<FString, int64_t> stats) {

        typedef std::pair<FString, int64_t> StatPair;
        foreach (const StatPair &pair, stats)
        {
            hlogstream(HLOG_DEBUG, pair.first << " : " << pair.second);
        }
    }
};

class StatTest : public EnableStats<StatTest, Locals<StatTest, int64_t> >
{
public:
    StatTest()
        : counter (0) {

        registerStatVariable<0>("counter", &StatTest::counter);
    }

    void DoSomethingToIncrement(void) {
        counter++;
    }

    void DoSomethingToDecrement(void) {
        counter--;
    }

protected:
    // StatVariable counter;
    int64_t counter;

};

TEST_F(EnableStatsUnitTest, SimpleStats)
{
    StatTest test;

    test.DoSomethingToIncrement();

    std::map<FString, int64_t> allStats =
        test.GetAllStats();

    ASSERT_EQ(1, allStats.size());
    ASSERT_EQ(1, allStats["counter"]);
    ASSERT_EQ(1, test.GetStat("counter"));

    test.DoSomethingToIncrement();
    ASSERT_EQ(2, test.GetStat("counter"));

    test.DoSomethingToDecrement();
    ASSERT_EQ(1, test.GetStat("counter"));

    test.DoSomethingToDecrement();
    ASSERT_EQ(0, test.GetStat("counter"));

    PrintStats(allStats);
}

class StatHierarchyTest :
    public EnableStats<
        StatHierarchyTest,
        Locals<StatHierarchyTest, int64_t, double, int64_t, double> >
{
public:
    StatHierarchyTest() :
        numberOfTestObjects (0),
        counter1 (0),
        counter2 (0),
        counter3 (0) {

        registerStatVariable<0>("numberOfTestObjects", &StatHierarchyTest::numberOfTestObjects);
        registerStatVariable<1>("counter1", &StatHierarchyTest::counter1);
        registerStatVariable<2>("counter2", &StatHierarchyTest::counter2);
        registerStatVariable<3>("counter3", &StatHierarchyTest::counter3);
    }


    void AddSubStatObject(void) {
        boost::shared_ptr<StatTest> obj (new StatTest());

        FString statObjectIdentifier =
            "StatTest" +
            FString(mStatTestVec.size() + 1);

        obj->AddChildTo(this, statObjectIdentifier);
        mStatTestVec.push_back(obj);
        numberOfTestObjects++;
        counter1 += 2;
    }

    void RemoveSubStatObject(void) {
        mStatTestVec.pop_back();
        numberOfTestObjects--;
        counter1 -= 2;
    }

protected:
    int64_t numberOfTestObjects;
    double counter1;
    int64_t counter2;
    double counter3;
    std::vector<boost::shared_ptr<StatTest> > mStatTestVec;

};

TEST_F(EnableStatsUnitTest, Hierarchy)
{
    StatHierarchyTest test;

    test.AddSubStatObject();

    std::map<FString, int64_t> allStats =
        test.GetAllStats();
    ASSERT_EQ(5, allStats.size());
    ASSERT_EQ(0, test.GetStat("StatTest1.counter"));
    PrintStats(allStats);

    test.AddSubStatObject();

    allStats = test.GetAllStats();
    ASSERT_EQ(6, allStats.size());
    ASSERT_EQ(0, test.GetStat("StatTest1.counter"));
    ASSERT_EQ(0, test.GetStat("StatTest2.counter"));
    PrintStats(allStats);

    test.RemoveSubStatObject();

    allStats = test.GetAllStats();
    ASSERT_EQ(5, allStats.size());
    PrintStats(allStats);

    test.RemoveSubStatObject();

    allStats = test.GetAllStats();
    ASSERT_EQ(4, allStats.size());
    PrintStats(allStats);

}

class AbstractClass : public virtual BaseEnableStats {
public:
    AbstractClass() {};
    virtual ~AbstractClass() {};

    virtual void doSomething() = 0;
};

class ConcreteClass :
    public AbstractClass,
    public EnableStats<ConcreteClass, Locals<ConcreteClass, int64_t> >
{
public:
    ConcreteClass()
        : counter (0) {

        registerStatVariable<0>("counter", &ConcreteClass::counter);
    }

    virtual void doSomething() {
        counter++;
    }

private:
    int64_t counter;
};

TEST_F(EnableStatsUnitTest, InheritenceTest)
{
    boost::shared_ptr<AbstractClass> obj( new ConcreteClass() );

    std::map<FString, int64_t> allStats = obj->GetAllStats();
    PrintStats(allStats);
}

class StatHierarchyOfAbstractTest :
    public EnableStats<StatHierarchyOfAbstractTest,
                       Locals<StatHierarchyOfAbstractTest, int64_t> >
{
public:
    StatHierarchyOfAbstractTest()
        : numberOfObjects (0) {

        registerStatVariable<0>("numberOfObjects", &StatHierarchyOfAbstractTest::numberOfObjects);
    }

    void AddObject(void) {
        boost::shared_ptr<AbstractClass> obj( new ConcreteClass() );
        obj->AddChildTo(
            this,
            FString(FStringFC(), "Child%ld", numberOfObjects));

        numberOfObjects++;
        for (int i =0 ; i < numberOfObjects; ++i)
            obj->doSomething();

        objects.push_back(obj);
    }

    void RemoveObject(void) {
        objects.pop_back();
        numberOfObjects--;
    }

protected:
    int64_t numberOfObjects;
    std::vector<boost::shared_ptr<AbstractClass> > objects;
};

TEST_F(EnableStatsUnitTest, HierarchyOfAbstractTypesTest)
{
    StatHierarchyOfAbstractTest test;

    std::map<FString, int64_t> allStats = test.GetAllStats();
    ASSERT_EQ(1, allStats.size());
    ASSERT_EQ(0, allStats["numberOfObjects"]);
    PrintStats(allStats);

    test.AddObject();
    allStats = test.GetAllStats();
    ASSERT_EQ(2, allStats.size());
    ASSERT_EQ(1, allStats["numberOfObjects"]);
    ASSERT_EQ(1, allStats["Child0.counter"]);
    PrintStats(allStats);

    test.AddObject();
    allStats = test.GetAllStats();
    ASSERT_EQ(3, allStats.size());
    ASSERT_EQ(2, allStats["numberOfObjects"]);
    ASSERT_EQ(1, allStats["Child0.counter"]);
    ASSERT_EQ(2, allStats["Child1.counter"]);
    PrintStats(allStats);

    test.RemoveObject();
    allStats = test.GetAllStats();
    ASSERT_EQ(2, allStats.size());
    ASSERT_EQ(1, allStats["numberOfObjects"]);
    ASSERT_EQ(1, allStats["Child0.counter"]);
    PrintStats(allStats);

    test.RemoveObject();
    allStats = test.GetAllStats();
    ASSERT_EQ(1, allStats.size());
    ASSERT_EQ(0, allStats["numberOfObjects"]);
    PrintStats(allStats);
}
