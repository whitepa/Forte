#include "ActiveObject.h"
#include "Clock.h"
#include "EventQueue.h"
#include "FileSystem.h"
#include "FTrace.h"
#include "LogManager.h"
#include "Util.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

using namespace Forte;
using namespace boost;

LogManager logManager;

class ActiveObjectTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.BeginLogging("//stdout", HLOG_ALL);
    };
    static void TearDownTestCase() {

    };
};


EXCEPTION_CLASS(EActiveTester);
EXCEPTION_CLASS(EActiveTesterCorrectException);
EXCEPTION_CLASS(EActiveTesterCancelled);

class ActiveTester : protected ActiveObject
{
public:
    ActiveTester() {};
    virtual ~ActiveTester() {};

    shared_ptr<Future<int> > PerformOneSecondOperationNullary(void) {
        return InvokeAsync<int>(boost::bind(&ActiveTester::OneSecondOperationNullary, this));
    };
    shared_ptr<Future<int> > PerformAddTwoNumbersVerySlowly(int a, int b) {
        return InvokeAsync<int>(boost::bind(&ActiveTester::AddTwoNumbersVerySlowly, this, a, b));
    };
    shared_ptr<Future<int> > PerformAddTwoNumbersVerySlowlyCancellable(int a, int b) {
        return InvokeAsync<int>(boost::bind(&ActiveTester::AddTwoNumbersVerySlowlyCancellable, this, a, b));
    };
    shared_ptr<Future<void> > PerformThrowingOperation(void) {
        return InvokeAsync<void>(boost::bind(&ActiveTester::ThrowingOperation, this));
    };
    shared_ptr<Future<void> > PerformThrowIncorrectly(void) {
        return InvokeAsync<void>(boost::bind(&ActiveTester::ThrowIncorrectly, this));
    };

protected:
    int OneSecondOperationNullary(void) { sleep(1); return 1; }
    int AddTwoNumbersVerySlowly(int a, int b) { sleep(1); return a + b; }
    int AddTwoNumbersVerySlowlyCancellable(int a, int b) { 
        int i = 0; 
        while (i++ < 10)
        {
            if (IsCancelled()) boost::throw_exception(EActiveTesterCancelled());
            usleep(100000);
        }
        return a + b;
    }
    void ThrowingOperation(void) { boost::throw_exception(EActiveTesterCorrectException()); }
    void ThrowIncorrectly(void) { throw EActiveTesterCorrectException(); }
};


TEST_F(ActiveObjectTest, Simple)
{
    ActiveTester a;
    
    shared_ptr<Future<int> > future = a.PerformOneSecondOperationNullary();
    ASSERT_EQ(false, future->IsReady());
    usleep(1100000); // 1.1 sec
    ASSERT_EQ(true, future->IsReady());    
    ASSERT_EQ(1, future->GetResult());
};

TEST_F(ActiveObjectTest, BlockingGetResult)
{
    ActiveTester a;
    
    MonotonicClock c;
    struct timespec start, end;
    start=c.GetTime();
    shared_ptr<Future<int> > future = a.PerformOneSecondOperationNullary();
    ASSERT_EQ(false, future->IsReady());
    ASSERT_EQ(1, future->GetResult());
    end = c.GetTime();
    ASSERT_TRUE(Timespec::FromSeconds(1) < 
                Timespec(end - start));
};

TEST_F(ActiveObjectTest, GetResultThrowsCorrectException)
{
    ActiveTester a;
    shared_ptr<Future<void> > future = a.PerformThrowingOperation();
    ASSERT_THROW(future->GetResult(), EActiveTesterCorrectException);
};
TEST_F(ActiveObjectTest, GetResultThrowsUnknownException)
{
    ActiveTester a;
    shared_ptr<Future<void> > future = a.PerformThrowIncorrectly();
    ASSERT_THROW(future->GetResult(), EFutureExceptionUnknown);
};
TEST_F(ActiveObjectTest, SimpleParameters)
{
    ActiveTester a;
    shared_ptr<Future<int> > future = a.PerformAddTwoNumbersVerySlowly(1,2);
    ASSERT_EQ(3, future->GetResult());
};
TEST_F(ActiveObjectTest, Cancellation)
{
    ActiveTester a;
    shared_ptr<Future<int> > future = a.PerformAddTwoNumbersVerySlowlyCancellable(1,2);
    ASSERT_EQ(false, future->IsReady());
    ASSERT_EQ(3, future->GetResult());
    future = a.PerformAddTwoNumbersVerySlowlyCancellable(1,2);
    ASSERT_EQ(false, future->IsReady());
    future->Cancel();
    ASSERT_THROW(future->GetResult(), EActiveTesterCancelled);
}
