#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "SSHRunner.h"

using namespace Forte;

LogManager mLogManager;

class SSHRunnerTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        mLogManager.BeginLogging("//stdout", HLOG_ALL);
    }
    static void TearDownTestCase() {
    }

    void SetUp() {
    }
    void TearDown() {
    }
};

/*
TEST_F(SSHRunnerTest, AuthenticateUsingPublicKey)
{
    SSHRunner ssh("root", "/opt/scale/lib/qa/.ssh/scale_computing_id_dsa.pub",
                  "/opt/scale/lib/qa/.ssh/scale_computing_id_dsa",
                  "w0rmh0l3backwards", "127.0.0.1", 22);
    FString out;
    FString errOut;
    ssh.Run("echo hi", &out, &errOut);
    hlog(HLOG_DEBUG, "out is '%s'", out.c_str());
    ASSERT_TRUE(FString("hi") == out.Trim());
}
*/

// TODO: move this test out of forte
TEST_F(SSHRunnerTest, AuthenticateUsingUsernamePassword)
{
    SSHRunner ssh("root", "storage", "127.0.0.1", 22);
    FString out;
    FString errOut;
    ssh.Run("echo hi", &out, &errOut);
    hlog(HLOG_DEBUG, "out is '%s'", out.c_str());
    ASSERT_TRUE(FString("hi") == out.Trim());
}
