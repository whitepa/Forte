#include <gtest/gtest.h>
#include "Exception.h"
#include "FTrace.h"

using namespace Forte;
using namespace std;

LogManager logManager;

class ExceptionTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.BeginLogging("exceptiontestlog.log");
    }

    static void TearDownTestCase() {
    }


    void SetUp() {

    }
};

class Dummy
{
public:
    Dummy() {}
    virtual ~Dummy() {}
    
    void throwForteException(const FString& description) {
        throw Exception(description);
    }
};

TEST_F(ExceptionTest, StdExceptionWhatIsOverriden)
{
    FTRACE;

    try
    {
        throw Exception("test");
    }
    catch (Exception &e)
    {
        ASSERT_STREQ(e.what(), "test");
    }
}

TEST_F(ExceptionTest, ForteExceptionIsAStdException)
{
    FTRACE;
    Dummy d;
    ASSERT_THROW(d.throwForteException("test"), exception);
}
