#include <gtest/gtest.h>
#include "ContextImpl.h"
#include "LogManager.h"
#include "ServiceConfig.h"

using namespace Forte;
using namespace boost;

ContextImpl mContext;
LogManager logManager;

class ServiceConfigUnitTest : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        logManager.BeginLogging("//stdout");
        logManager.SetLogMask("//stdout", HLOG_ALL);
        hlog(HLOG_DEBUG, "Starting test...");
    }

    static void TearDownTestCase()
    {
        logManager.EndLogging("//stdout");
    }
};

TEST_F(ServiceConfigUnitTest, CopyTreeTest)
{
    CGETNEW("forte.ServiceConfig", ServiceConfig, sc);
    sc.ReadConfigFile("./ServiceConfigUnitTest.conf");

    // Copy over an existing tree
    FString g1o2 = sc.Get<FString>("group1.one.two");
    FString g2o2 = sc.Get<FString>("group2.one.two");
    ASSERT_TRUE(g1o2 != g2o2);

    sc.CopyTree("group1.one", "group2.one");
    FString g2o2new = sc.Get<FString>("group2.one.two");
    ASSERT_TRUE(g1o2 == g2o2new);

    // Copy to a new tree
    FString g1fb = sc.Get<FString>("group1.foo.bar");
    FString g2fb;
    try
    {
        g2fb = sc.Get<FString>("group2.foo.bar");
    }
    catch (std::exception &e)
    {
        // Expected to be non-existent. Ignore
    }
    ASSERT_TRUE(g2fb.empty());

    sc.CopyTree("group1.foo", "group2.foo");
    g2fb = sc.Get<FString>("group2.foo.bar");
    ASSERT_TRUE(g1fb == g2fb);
}

TEST_F(ServiceConfigUnitTest, ResolveDuplicates)
{
    CGETNEW("forte.ServiceConfig", ServiceConfig, sc);
    sc.ReadConfigFile("./ServiceConfigUnitTest.conf");

    // Assume conf contains specific duplicates
    FString newG1ID = sc.Get<FString>("group1.id");
    ASSERT_TRUE(newG1ID == "13");
}
