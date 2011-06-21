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
    void boostThrowForteException(const FString& description) {
        boost::throw_exception(Exception(description));
    }
};

TEST_F(ExceptionTest, StdExceptionWhatIsOverriden)
{
    FTRACE;

    try
    {
        boost::throw_exception(Exception("test"));
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

TEST_F(ExceptionTest, ForteExThrownByBoostIsAStdException)
{
    FTRACE;
    Dummy d;
    ASSERT_THROW(d.boostThrowForteException("test"), exception);
}

EXCEPTION_CLASS(EExceptionUnitTest);
EXCEPTION_SUBCLASS(EExceptionUnitTest, EExceptionUnitTestSubClass);
EXCEPTION_SUBCLASS2(EExceptionUnitTest, EExceptionUnitTestSubClass2, "EExceptionUnitTestSubClass2 Description");

TEST_F(ExceptionTest, ClassMacroTestBase)
{
    FTRACE;
    try
    {
        throw EExceptionUnitTest();
    }
    catch (EExceptionUnitTest& e)
    {
        hlog(HLOG_INFO, "Exception is %s", e.what());
        ASSERT_STREQ(e.what(), "EExceptionUnitTest");
    }

    try
    {
        throw EExceptionUnitTest("SomeText");
    }
    catch (EExceptionUnitTest& e)
    {
        hlog(HLOG_INFO, "Exception is %s", e.what());
        ASSERT_STREQ(e.what(), "SomeText");
    }

    try
    {
        throw EExceptionUnitTest(FStringFC(), "SomeText %s %d", "one", 1);
    }
    catch (EExceptionUnitTest& e)
    {
        hlog(HLOG_INFO, "Exception is %s", e.what());
        ASSERT_STREQ(e.what(), "SomeText one 1");
    }
}

TEST_F(ExceptionTest, ClassMacroTestSub)
{
    FTRACE;
    try
    {
        throw EExceptionUnitTestSubClass();
    }
    catch (EExceptionUnitTestSubClass& e)
    {
        hlog(HLOG_INFO, "Exception is %s", e.what());
        ASSERT_STREQ(e.what(), "EExceptionUnitTestSubClass");
    }

    try
    {
        throw EExceptionUnitTestSubClass("Some text");
    }
    catch (EExceptionUnitTestSubClass& e)
    {
        hlog(HLOG_INFO, "Exception is %s", e.what());
        ASSERT_STREQ(e.what(), "Some text");
    }

    try
    {
        throw EExceptionUnitTestSubClass(FStringFC(), "%s %d", "Some text", 10);
    }
    catch (EExceptionUnitTestSubClass& e)
    {
        hlog(HLOG_INFO, "Exception is %s", e.what());
        ASSERT_STREQ(e.what(), "Some text 10");
    }
}

TEST_F(ExceptionTest, ClassMacroTestSub2)
{
    FTRACE;
    try
    {
        throw EExceptionUnitTestSubClass2();
    }
    catch (EExceptionUnitTestSubClass2& e)
    {
        hlog(HLOG_INFO, "Exception is %s", e.what());
        ASSERT_STREQ(e.what(), "EExceptionUnitTestSubClass2 Description");
    }

    try
    {
        throw EExceptionUnitTestSubClass2("Some text");
    }
    catch (EExceptionUnitTestSubClass2& e)
    {
        hlog(HLOG_INFO, "Exception is %s", e.what());
        ASSERT_STREQ(e.what(), "EExceptionUnitTestSubClass2 Description: Some text");
    }

    try
    {
        throw EExceptionUnitTestSubClass2(FStringFC(), "%s %d", "Some text", 10);
    }
    catch (EExceptionUnitTestSubClass2& e)
    {
        hlog(HLOG_INFO, "Exception is %s", e.what());
        ASSERT_STREQ(e.what(), "EExceptionUnitTestSubClass2 Description: Some text 10");
    }
}

TEST_F(ExceptionTest, hlogAndThrow)
{
    FTRACE;
    try
    {
        hlog_and_throw(HLOG_INFO, EExceptionUnitTest());
        FAIL() << "hlog_and_throw failed to throw";
    }
    catch (EExceptionUnitTest& e)
    {
        hlog(HLOG_INFO, "Exception is %s", e.what());
        ASSERT_STREQ(e.what(), "EExceptionUnitTest");
    }
}
