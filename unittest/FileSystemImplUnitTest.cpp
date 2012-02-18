
#include <gtest/gtest.h>
#include "LogManager.h"
#include "FileSystemImpl.h"
#include "SystemCallUtil.h"

using namespace Forte;

LogManager logManager;

class FileSystemImplUnitTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");
        hlog(HLOG_DEBUG, "Starting test...");
    }
};
TEST_F(FileSystemImplUnitTest, StatFS)
{
    hlog(HLOG_INFO, "StatFS");
    FileSystemImpl f;
    struct statfs st;
    f.StatFS("/", &st);
}

TEST_F(FileSystemImplUnitTest, StatFSPathDoesNotExist)
{
    hlog(HLOG_INFO, "StatFSPathDoesNotExist");
    FileSystemImpl f;
    struct statfs st;
    ASSERT_THROW(f.StatFS("pathdoesnotexist", &st), EErrNoENOENT);
}

TEST_F(FileSystemImplUnitTest, ScanDirReplacement)
{
    hlog(HLOG_INFO, "ScanDirReplacement");
    FileSystemImpl f;

    vector<FString> names;
    f.ScanDir("/",names);
    ASSERT_NE(names.size(), 0);
}

TEST_F(FileSystemImplUnitTest, TouchFileAndTestTimes)
{
    hlog(HLOG_INFO, "TouchFileAndTestTimes");
    FileSystemImpl f;
    FString fileName = "/tmp/TouchFileAndTestTimes_unittest_file";

    struct stat st;
    long long int currTime = (int64_t)time(0);
    f.Touch(fileName);

    if (f.Stat(fileName, &st))
    {
        hlog(HLOG_ERR, "Stat failed on %s, err = %d", fileName.c_str(), errno);
    }
    else
    {
        hlog(HLOG_INFO, "currTime=%lld aTime=%lld mTime=%lld", currTime,
                 (long long int)st.st_atime, (long long int)st.st_mtime);
        ASSERT_LT(abs(st.st_atime - currTime), 2);
        ASSERT_LT(abs(st.st_mtime - currTime), 2);
    }
}

TEST_F(FileSystemImplUnitTest, TestFileCopy)
{
    hlog(HLOG_INFO, "TestFileCopy");
    FileSystemImpl f;

    FString fileName1 = "/tmp/TestFileCopy_unittest_file1";
    FString fileName2 = "/tmp/TestFileCopy_unittest_file2";

    FString contents1 = "contents1\n";

    f.FilePutContents(fileName1, contents1);

    f.FileCopy(fileName1, fileName2);


    FString contents2 = f.FileGetContents(fileName2);

    f.Unlink(fileName1);
    f.Unlink(fileName2);

    hlog(HLOG_INFO, "File Contents %s", contents2.c_str());
    ASSERT_EQ(contents1, contents2);

}

TEST_F(FileSystemImplUnitTest, TestFileAppend)
{
    hlog(HLOG_INFO, "TestFileAppend");

    FileSystemImpl f;
    FString fileName1 = "/tmp/TestFileConcatonation_unittest_file1";
    FString fileName2 = "/tmp/TestFileConcatonation_unittest_file2";

    FString contents1 = "contents1\nsecondline(contents1)\n";
    FString contents2 = "contents2\nsecondline(contents2)\n";

    f.FilePutContents(fileName1, contents1);
    f.FilePutContents(fileName2, contents2);

    f.FileAppend(fileName2, fileName1);

    FString contents3 = f.FileGetContents(fileName1);

    f.Unlink(fileName1);
    f.Unlink(fileName2);
    hlog(HLOG_INFO, "File Contents %s", contents3.c_str());
    ASSERT_EQ(contents3, (contents1 + contents2));
}


TEST_F(FileSystemImplUnitTest, DirName)
{
    hlog(HLOG_INFO, "DireName");

    FileSystemImpl f;
    FString file1 = "/level1/level2/level3/level4.tmp";

    ASSERT_STREQ("/level1/level2/level3", f.Dirname(file1));

    FString file2 = "test.tmp";
    ASSERT_STREQ(".", f.Dirname(file2));
}

TEST_F(FileSystemImplUnitTest, Basename)
{
    hlog(HLOG_INFO, "Basename");

    FileSystemImpl f;
    FString file1 = "/level1/level2/level3/level4.tmp";

    ASSERT_STREQ("level4.tmp", f.Basename(file1));
    ASSERT_STREQ("level4", f.Basename(file1, ".tmp"));

    FString file2 = "test.tmp";
    ASSERT_STREQ("test.tmp", f.Basename(file2));
    ASSERT_STREQ("test", f.Basename(file2, ".tmp"));
}


