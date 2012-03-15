#include <gtest/gtest.h>
#include "LogManager.h"
#include "FTrace.h"

using namespace std;
using namespace boost;
using namespace Forte;


LogManager logManager;

void outputAllMessagesFunction(int howDeep=1)
{
    FTRACE;
    hlog(HLOG_SQL, "HLOG_SQL");
    hlog(HLOG_TRACE, "HLOG_TRACE");
    hlog(HLOG_DEBUG4, "HLOG_DEBUG4");
    hlog(HLOG_DEBUG3, "HLOG_DEBUG3");
    hlog(HLOG_DEBUG2, "HLOG_DEBUG2");
    hlog(HLOG_DEBUG1, "HLOG_DEBUG1");
    hlog(HLOG_DEBUG, "HLOG_DEBUG");
    hlog(HLOG_INFO, "HLOG_INFO");
    hlog(HLOG_NOTICE, "HLOG_NOTICE");
    hlog(HLOG_WARN, "HLOG_WARN");
    hlog(HLOG_ERR, "HLOG_ERR");
    hlog(HLOG_CRIT, "HLOG_CRIT");
    hlog(HLOG_ALERT, "HLOG_ALERT");
    hlog(HLOG_EMERG, "HLOG_EMERG");

    if (howDeep > 0)
    {
        outputAllMessagesFunction(howDeep-1);
    }
}

TEST(LogManagerUnitTest, WatchOutput)
{
    logManager.BeginLogging("//stdout",
                            HLOG_NODEBUG,
                            HLOG_FORMAT_TIMESTAMP | HLOG_FORMAT_MESSAGE);

    outputAllMessagesFunction();

    logManager.EndLogging("//stdout");
}

TEST(LogManagerUnitTest, WatchOutputWithDepth)
{
    logManager.BeginLogging(
        "//stdout", HLOG_NODEBUG,
        HLOG_FORMAT_TIMESTAMP | HLOG_FORMAT_MESSAGE | HLOG_FORMAT_DEPTH);

    outputAllMessagesFunction(3);

    logManager.EndLogging("//stdout");
}

TEST(LogManagerUnitTest, TestOutput)
{
    FString filename = "./testoutputlog";
    logManager.BeginLogging(filename,
                            HLOG_NODEBUG,
                            HLOG_FORMAT_TIMESTAMP | HLOG_FORMAT_MESSAGE);

    outputAllMessagesFunction();

    EXPECT_EQ(0, system("/bin/grep CRIT ./testoutputlog"));
    EXPECT_NE(0, system("/bin/grep DEBUG ./testoutputlog"));

    logManager.EndLogging(filename);
    unlink(filename.c_str());
}
