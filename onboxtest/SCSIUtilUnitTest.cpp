// Copyright (c) 2008-2010 Scale Computing, Inc.  All Rights Reserved.
// Proprietary and Confidential

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "LogManager.h"
#include "SCSIUtil.h"
#include "FileSystemImpl.h"

// #SCQAD TESTTAG: broken, smoketest, forte, storage, storage.iscsi

using namespace Forte;

LogManager logManager;
// -----------------------------------------------------------------------------

class SCSIDriveUnitTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.BeginLogging("//stdout", HLOG_ALL);
    }

    static void TearDownTestCase() {
    }

    void SetUp() {
    }

    void TearDown() {
    }
};
// -----------------------------------------------------------------------------

TEST_F(SCSIDriveUnitTest, GetDeviceInfoDevSda)
{
    ProcRunner  pr;
    FileSystemImpl fs;
    FString     dev_path("/dev/sda");
    int         host1 = -1;
    int         lun1  = -1;

    SCSIUtil su(pr, fs);

    su.GetDeviceInfo(dev_path, host1, lun1);

    hlog(HLOG_DEBUG, "dev: %s, slot: %d, lun: %d",
                     dev_path.c_str(), host1, lun1);

    EXPECT_EQ(0, host1);
    EXPECT_EQ(0, lun1);
}
// -----------------------------------------------------------------------------

TEST_F(SCSIDriveUnitTest, GetDeviceInfoDevSdb)
{
    ProcRunner  pr;
    FileSystemImpl fs;
    FString     dev_path("/dev/sdb");
    int         host1 = -1;
    int         lun1  = -1;

    SCSIUtil  su(pr, fs);

    su.GetDeviceInfo(dev_path, host1, lun1);

    hlog(HLOG_DEBUG, "dev: %s, slot: %d, lun: %d",
                     dev_path.c_str(), host1, lun1);

    EXPECT_EQ(1, host1);
    EXPECT_EQ(0, lun1);
}
// -----------------------------------------------------------------------------
