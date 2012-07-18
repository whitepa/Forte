// Copyright (c) 2008-2010 Scale Computing, Inc.  All Rights Reserved.
// Proprietary and Confidential

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "LogManager.h"
#include "SCSIUtil.h"
#include "FTrace.h"
#include "MockProcRunner.h"
#include "GMockFileSystem.h"

using namespace Forte;
using ::testing::_;
using ::testing::StrEq;
using ::testing::SetArgReferee;

LogManager logManager;
// -----------------------------------------------------------------------------

class SCSIUtilTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.BeginLogging("SCSIUtilUnitTest.log", HLOG_ALL);
    }

    static void TearDownTestCase() {
        logManager.EndLogging();
    }

    void SetUp() {
    }

    void TearDown() {
    }

    MockProcRunner mProcRunner;
    GMockFileSystem mFileSystem;
};

// -----------------------------------------------------------------------------

TEST_F(SCSIUtilTest, RescanHostExceptionWhenCommandFails)
{
    FTRACE;

    SCSIUtil su(mProcRunner, mFileSystem);
    mProcRunner.SetCommandResponse(
        "/bin/echo - - - > /sys/class/scsi_host/host0/scan", "", 1);
    ASSERT_THROW(su.RescanHost(0), ESCSIScanFailed);
}

TEST_F(SCSIUtilTest, RescanHostSuccessCase)
{
    FTRACE;

    SCSIUtil su(mProcRunner, mFileSystem);
    mProcRunner.SetCommandResponse(
        "/bin/echo - - - > /sys/class/scsi_host/host0/scan", "", 0);
    ASSERT_NO_THROW(su.RescanHost(0));
}

TEST_F(SCSIUtilTest, RescanAllHostsScansAll)
{
    FTRACE;

    SCSIUtil su(mProcRunner, mFileSystem);

    vector<FString> children;
    children.push_back("host0");
    children.push_back("host1");

    EXPECT_CALL(
        mFileSystem,
        GetChildren(StrEq("/sys/class/scsi_host"),_,_,_,_))
        .WillOnce(SetArgReferee<1>(children));


    mProcRunner.SetCommandResponse(
        "/bin/echo - - - > /sys/class/scsi_host/host0/scan", "", 0);

    mProcRunner.SetCommandResponse(
        "/bin/echo - - - > /sys/class/scsi_host/host1/scan", "", 0);

    ASSERT_NO_THROW(su.RescanAllHosts());
}

TEST_F(SCSIUtilTest, RescanAllHostsThrowsIfOneFails)
{
    FTRACE;

    SCSIUtil su(mProcRunner, mFileSystem);

    vector<FString> children;
    children.push_back("host0");
    children.push_back("host1");

    EXPECT_CALL(
        mFileSystem,
        GetChildren(StrEq("/sys/class/scsi_host"),_,_,_,_))
        .WillOnce(SetArgReferee<1>(children));


    mProcRunner.SetCommandResponse(
        "/bin/echo - - - > /sys/class/scsi_host/host0/scan", "", 0);

    mProcRunner.SetCommandResponse(
        "/bin/echo - - - > /sys/class/scsi_host/host1/scan", "", 1);


    ASSERT_THROW(su.RescanAllHosts(), ESCSIScanFailed);
}

TEST_F(SCSIUtilTest, RescanAllHostsThrowsIfOneFailsButTriesAll)
{
    FTRACE;

    SCSIUtil su(mProcRunner, mFileSystem);

    vector<FString> children;
    children.push_back("host0");
    children.push_back("host1");

    EXPECT_CALL(
        mFileSystem,
        GetChildren(StrEq("/sys/class/scsi_host"),_,_,_,_))
        .WillOnce(SetArgReferee<1>(children));


    mProcRunner.SetCommandResponse(
        "/bin/echo - - - > /sys/class/scsi_host/host0/scan", "", 1);

    mProcRunner.SetCommandResponse(
        "/bin/echo - - - > /sys/class/scsi_host/host1/scan", "", 0);


    ASSERT_THROW(su.RescanAllHosts(), ESCSIScanFailed);
    ASSERT_TRUE(mProcRunner.CommandWasRun(
                    "/bin/echo - - - > /sys/class/scsi_host/host0/scan"));
    ASSERT_TRUE(mProcRunner.CommandWasRun(
                    "/bin/echo - - - > /sys/class/scsi_host/host1/scan"));
}
