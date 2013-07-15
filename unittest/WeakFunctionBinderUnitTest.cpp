#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ContextImpl.h"
#include "Forte.h"
#include "WeakFunctionBinder.h"

using namespace Forte;
using namespace boost;


class WeakFunctionBinderUnitTest : public ::testing::Test
{
public:
    WeakFunctionBinderUnitTest() {}

    struct CallbackReceiver
    {
        CallbackReceiver() : mCalls(0) {}
        void Callback(void) {
            ++mCalls;
        }
        unsigned mCalls;
    };
};

TEST_F(WeakFunctionBinderUnitTest, CallbackWorks)
{
    boost::shared_ptr<CallbackReceiver> r(new CallbackReceiver());
    boost::function<void()> f = boost::bind(WeakFunctionBinder(
                                                &CallbackReceiver::Callback, r));
    EXPECT_NO_THROW(f());
    EXPECT_EQ(1, r->mCalls);
}

TEST_F(WeakFunctionBinderUnitTest, ThrowOnBadPtr)
{
    boost::shared_ptr<CallbackReceiver> r(new CallbackReceiver());
    boost::function<void()> f = boost::bind(WeakFunctionBinder(
                                                &CallbackReceiver::Callback, r));
    EXPECT_EQ(0, r->mCalls);
    EXPECT_NO_THROW(f());
    EXPECT_EQ(1, r->mCalls);
    r.reset();
    EXPECT_THROW(f(), EObjectCouldNotBeLocked);
}
