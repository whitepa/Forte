#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"

#include "AutoMutex.h"

using namespace std;
using namespace boost;
using namespace Forte;

LogManager logManager;

class AutoMutexUnitTest : public ::testing::Test
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

TEST_F(AutoMutexUnitTest, ConstructDelete)
{
    FTRACE;
    Mutex m;

    {
        AutoUnlockMutex lock(m);
    }

    m.Lock();

    {
        AutoLockMutex lock(m);
    }

    m.Unlock();
}
