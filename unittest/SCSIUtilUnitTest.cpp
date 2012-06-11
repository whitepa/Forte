// Copyright (c) 2008-2010 Scale Computing, Inc.  All Rights Reserved.
// Proprietary and Confidential

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "LogManager.h"
#include "SCSIUtil.h"
#include "FTrace.h"
#include "MockProcRunner.h"

using namespace Forte;

LogManager logManager;
// -----------------------------------------------------------------------------

class SCSIUtilTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.BeginLogging("SCSIUtilUnitTest.log", HLOG_ALL);
        mProcRunner = new MockProcRunner();
    }

    static void TearDownTestCase() {
        delete mProcRunner;
    }

    void SetUp() {
    }

    void TearDown() {
    }

    static MockProcRunner *mProcRunner;
};
MockProcRunner * SCSIUtilTest::mProcRunner = NULL;

// -----------------------------------------------------------------------------

TEST_F(SCSIUtilTest, RescanHostExceptionWhenCommandFails)
{
    FTRACE;

    SCSIUtil su(*mProcRunner);
    mProcRunner->SetCommandResponse(
        "/bin/echo - - - > /sys/class/scsi_host/host0/scan", "", 1);
    ASSERT_THROW(su.RescanHost(0), ESCSIScanFailed);
}

TEST_F(SCSIUtilTest, RescanHostSuccessCase)
{
    FTRACE;

    SCSIUtil su(*mProcRunner);
    mProcRunner->SetCommandResponse(
        "/bin/echo - - - > /sys/class/scsi_host/host0/scan", "", 0);
    ASSERT_NO_THROW(su.RescanHost(0));
}
