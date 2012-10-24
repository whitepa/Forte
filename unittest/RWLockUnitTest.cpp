#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"

#include "RWLock.h"

using namespace std;
using namespace boost;
using namespace Forte;

LogManager logManager;

class RWLockUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
        hlog(HLOG_DEBUG, "Starting test...");
    }

    static void TearDownTestCase() {
    }

    void SetUp() {
    }

    void TearDown() {
    }
};

TEST_F(RWLockUnitTest, ConstructDelete)
{
    FTRACE;
    RWLock rwl;

    {
        AutoReadUnlock lock(rwl);
    }

    {
        AutoWriteUnlock lock(rwl);
    }
}

TEST_F(RWLockUnitTest, MultipleReadLocks)
{
    FTRACE;
    RWLock rwl;

    {
        AutoReadUnlock lock(rwl);
        AutoReadUnlock lock2(rwl);
        AutoReadUnlock lock3(rwl);
    }
}
