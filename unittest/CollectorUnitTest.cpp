#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "CheckedValueStore.h"
#include "MockCheckedValueStore.h"
#include "Collector.h"
#include "CheckedInt32.h"
#include "ContextImpl.h"
#include <boost/make_shared.hpp>

using namespace Forte;

LogManager logManager;


static const char* CV_NAME = "cv.test";

class TestCollector : public Collector
{
public:
    TestCollector(Forte::Context &ctxt,
                  const Forte::CheckedValueStorePtr &cvStore, const char *name,
                  unsigned int id) :
            Collector(ctxt, cvStore, name), mID(id)
    {

    }

    void collectLocked()
    {
        trackCheckedValuesToKeep();
        boost::shared_ptr<CheckedInt32> test = getCheckedValue<CheckedInt32>(CV_NAME);
        test->Set(mID);
        pruneCheckedValuesNotTracked();
    }

    std::list<boost::function<void()> > mTasks;
    const unsigned int mID;

    using Forte::Collector::mCvSet;
    using Forte::Collector::takeOwnershipOfCheckedValue;

};

class CollectorUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
        hlog(HLOG_DEBUG, "Starting test...");

        mConfig=boost::make_shared<ServiceConfig>();
        mConfig->Set("checkedValue.default.expirationInterval", "60000");
        mConfig->Set("checkedValue.default.stringEnums", "");
        mConfig->Set("checkedValue.default.intRangeMin", "0");
        mConfig->Set("checkedValue.default.intRangeMax", "0");
        mConfig->Set("checkedValue.default.backingMedium", "MEMORY");
        mConfig->Set("zookeeper.port", "2181");
    }

    ContextImpl mContext;
    static ServiceConfigPtr mConfig;
};
ServiceConfigPtr CollectorUnitTest::mConfig;

TEST_F(CollectorUnitTest, TestCollect)
{
    boost::shared_ptr<MockCheckedValueStore> cvs = boost::make_shared<MockCheckedValueStore>(mConfig);
    boost::shared_ptr<TestCollector> collector(new TestCollector(mContext, cvs, "testCollector", 1));

    collector->Collect();
    ASSERT_TRUE(collector->mCvSet.find(CV_NAME) != collector->mCvSet.end() );
}

TEST_F(CollectorUnitTest, TestCollectorOwnershipChange)
{
    boost::shared_ptr<MockCheckedValueStore> cvs = boost::make_shared<MockCheckedValueStore>(mConfig);

    boost::shared_ptr<TestCollector> collector1(new TestCollector(mContext, cvs, "testCollector", 1));
    boost::shared_ptr<TestCollector> collector2(new TestCollector(mContext, cvs, "testCollector", 2));

    collector1->Collect();
    collector2->Collect();
    ASSERT_THROW(collector1->Collect(), ECollectorCannotRetakeOwnership);

    collector1->takeOwnershipOfCheckedValue(CV_NAME);
    ASSERT_NO_THROW(collector1->Collect());
    ASSERT_THROW(collector2->Collect(), ECollectorCannotRetakeOwnership);

    ASSERT_TRUE(collector1->mCvSet.find(CV_NAME) != collector1->mCvSet.end() );
    ASSERT_TRUE(collector2->mCvSet.find(CV_NAME) != collector2->mCvSet.end() );

    collector1.reset();
    ASSERT_NO_THROW(collector2->Collect());
}
