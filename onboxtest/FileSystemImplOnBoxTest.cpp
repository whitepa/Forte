// #SCQAD TEST: ONBOX: FileSystemImplOnBoxTest
// #SCQAD TESTTAG: smoketest, forte
#include <gtest/gtest.h>
#include "LogManager.h"
#include "FileSystemImpl.h"
#include "SystemCallUtil.h"

using namespace std;
using namespace Forte;

LogManager logManager;

class FileSystemImplOnBoxTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");
        hlog(HLOG_DEBUG, "Starting test...");
    }
};
TEST_F(FileSystemImplOnBoxTest, StatFS)
{
    hlog(HLOG_INFO, "StatFS");
    FileSystemImpl f;
    struct statfs st;
    f.StatFS("/", &st);
}

TEST_F(FileSystemImplOnBoxTest, StatFSPathDoesNotExist)
{
    hlog(HLOG_INFO, "StatFSPathDoesNotExist");
    FileSystemImpl f;
    struct statfs st;
    ASSERT_THROW(f.StatFS("pathdoesnotexist", &st), EErrNoENOENT);
}

TEST_F(FileSystemImplOnBoxTest, ScanDirReplacement)
{
    hlog(HLOG_INFO, "ScanDirReplacement");
    FileSystemImpl f;

    vector<FString> names;
    f.ScanDir("/",names);
    ASSERT_NE(names.size(), 0);
}

TEST_F(FileSystemImplOnBoxTest, TouchFileAndTestTimes)
{
    hlog(HLOG_INFO, "TouchFileAndTestTimes");
    FileSystemImpl f;
    FString fileName = "/tmp/TouchFileAndTestTimes_onboxtest_file";

    struct stat st;
    long long int currTime = static_cast<int64_t>(time(0));
    f.Touch(fileName);

    if (f.Stat(fileName, &st))
    {
        hlog(HLOG_ERR, "Stat failed on %s, err = %d", fileName.c_str(), errno);
    }
    else
    {
        hlog(HLOG_INFO, "currTime=%lld aTime=%lld mTime=%lld", currTime,
             static_cast<long long int>(st.st_atime),
             static_cast<long long int>(st.st_mtime));
        ASSERT_LT(abs(st.st_atime - currTime), 2);
        ASSERT_LT(abs(st.st_mtime - currTime), 2);
    }
}

TEST_F(FileSystemImplOnBoxTest, TestFileCopy)
{
    hlog(HLOG_INFO, "TestFileCopy");
    FileSystemImpl f;

    FString fileName1 = "/tmp/TestFileCopy_onboxtest_file1";
    FString fileName2 = "/tmp/TestFileCopy_onboxtest_file2";

    FString contents1 = "contents1\n";

    f.FilePutContents(fileName1, contents1);

    f.FileCopy(fileName1, fileName2);


    FString contents2 = f.FileGetContents(fileName2);

    f.Unlink(fileName1);
    f.Unlink(fileName2);

    hlog(HLOG_INFO, "File Contents %s", contents2.c_str());
    ASSERT_EQ(contents1, contents2);

}

TEST_F(FileSystemImplOnBoxTest, TestFileAppend)
{
    hlog(HLOG_INFO, "TestFileAppend");

    FileSystemImpl f;
    FString fileName1 = "/tmp/TestFileConcatonation_onboxtest_file1";
    FString fileName2 = "/tmp/TestFileConcatonation_onboxtest_file2";

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


TEST_F(FileSystemImplOnBoxTest, DirName)
{
    hlog(HLOG_INFO, "DireName");

    FileSystemImpl f;
    FString file1 = "/level1/level2/level3/level4.tmp";

    ASSERT_STREQ("/level1/level2/level3", f.Dirname(file1));

    FString file2 = "test.tmp";
    ASSERT_STREQ(".", f.Dirname(file2));
}

TEST_F(FileSystemImplOnBoxTest, Basename)
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

TEST_F(FileSystemImplOnBoxTest, MakeTemporaryFile)
{
    hlog(HLOG_INFO, "MakeTemporaryFile");

    FileSystemImpl f;
    FString templateFile = "/tmp/tempXXXXXX";
    FString tmpFile;

    {
        tmpFile = f.MakeTemporaryFile(templateFile);
        ASSERT_STRNE(tmpFile, templateFile);
        ASSERT_TRUE(f.FileExists(tmpFile));
    }
    ASSERT_TRUE(f.FileExists(tmpFile));
    f.Unlink(tmpFile);
}

TEST_F(FileSystemImplOnBoxTest, GetChildren_NoRecurse_NoPath_NoDirNames)
{
    FileSystemImpl f;

    f.MakeDir("/tmp/root");
    f.Touch("/tmp/root/file0");
    f.Touch("/tmp/root/file1");
    f.MakeDir("/tmp/root/dir0");
    f.MakeDir("/tmp/root/dir1");
    f.Touch("/tmp/root/dir0/file2");
    f.MakeDir("/tmp/root/dir0/dir2");

    vector<FString> children;
    f.GetChildren("/tmp/root", children, false, false, false);

    EXPECT_EQ(2, children.size());

    vector<FString>::const_iterator end(children.end());

    EXPECT_TRUE(find(children.begin(), children.end(), "file0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "file1") != end);
}

TEST_F(FileSystemImplOnBoxTest, GetChildren_NoRecurse_YesPath_NoDirNames)
{
    FileSystemImpl f;

    f.MakeDir("/tmp/root");
    f.Touch("/tmp/root/file0");
    f.Touch("/tmp/root/file1");
    f.MakeDir("/tmp/root/dir0");
    f.MakeDir("/tmp/root/dir1");
    f.Touch("/tmp/root/dir0/file2");
    f.MakeDir("/tmp/root/dir0/dir2");

    vector<FString> children;
    f.GetChildren("/tmp/root", children, false, true, false);

    EXPECT_EQ(2, children.size());

    vector<FString>::const_iterator end(children.end());

    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/file0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/file1") != end);
}

TEST_F(FileSystemImplOnBoxTest, GetChildren_YesRecurse_NoPath_NoDirNames)
{
    FileSystemImpl f;

    f.MakeDir("/tmp/root");
    f.Touch("/tmp/root/file0");
    f.Touch("/tmp/root/file1");
    f.MakeDir("/tmp/root/dir0");
    f.MakeDir("/tmp/root/dir1");
    f.Touch("/tmp/root/dir0/file2");
    f.MakeDir("/tmp/root/dir0/dir2");

    vector<FString> children;
    f.GetChildren("/tmp/root", children, true, false, false);

    EXPECT_EQ(3, children.size());

    vector<FString>::const_iterator end(children.end());

    EXPECT_TRUE(find(children.begin(), children.end(), "file0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "file1") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "file2") != end);
}

TEST_F(FileSystemImplOnBoxTest, GetChildren_YesRecurse_YesPath_NoDirNames)
{
    FileSystemImpl f;

    f.MakeDir("/tmp/root");
    f.Touch("/tmp/root/file0");
    f.Touch("/tmp/root/file1");
    f.MakeDir("/tmp/root/dir0");
    f.MakeDir("/tmp/root/dir1");
    f.Touch("/tmp/root/dir0/file2");
    f.MakeDir("/tmp/root/dir0/dir2");

    vector<FString> children;
    f.GetChildren("/tmp/root", children, true, true, false);

    EXPECT_EQ(3, children.size());

    foreach(FString& s, children)
    {
        cout << s;
    }

    vector<FString>::const_iterator end(children.end());

    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/file0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/file1") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/dir0/file2") != end);
}

TEST_F(FileSystemImplOnBoxTest, GetChildren_NoRecurse_NoPaths_YesDirNames)
{
    FileSystemImpl f;

    f.MakeDir("/tmp/root");
    f.Touch("/tmp/root/file0");
    f.Touch("/tmp/root/file1");
    f.MakeDir("/tmp/root/dir0");
    f.MakeDir("/tmp/root/dir1");
    f.Touch("/tmp/root/dir0/file2");
    f.MakeDir("/tmp/root/dir0/dir2");

    vector<FString> children;
    f.GetChildren("/tmp/root", children, false, false, true);

    EXPECT_EQ(4, children.size());

    vector<FString>::const_iterator end(children.end());

    EXPECT_TRUE(find(children.begin(), children.end(), "file0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "file1") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "dir0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "dir1") != end);
}

TEST_F(FileSystemImplOnBoxTest, GetChildren_NoRecurse_YesPaths_YesDirNames)
{
    FileSystemImpl f;

    f.MakeDir("/tmp/root");
    f.Touch("/tmp/root/file0");
    f.Touch("/tmp/root/file1");
    f.MakeDir("/tmp/root/dir0");
    f.MakeDir("/tmp/root/dir1");
    f.Touch("/tmp/root/dir0/file2");
    f.MakeDir("/tmp/root/dir0/dir2");

    vector<FString> children;
    f.GetChildren("/tmp/root", children, false, true, true);

    EXPECT_EQ(4, children.size());

    vector<FString>::const_iterator end(children.end());

    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/file0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/file1") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/dir0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/dir1") != end);
}

TEST_F(FileSystemImplOnBoxTest, GetChildren_Recurse_NoPaths_YesDirNames)
{
    FileSystemImpl f;

    f.MakeDir("/tmp/root");
    f.Touch("/tmp/root/file0");
    f.Touch("/tmp/root/file1");
    f.MakeDir("/tmp/root/dir0");
    f.MakeDir("/tmp/root/dir1");
    f.Touch("/tmp/root/dir0/file2");
    f.MakeDir("/tmp/root/dir0/dir2");

    vector<FString> children;
    f.GetChildren("/tmp/root", children, true, false, true);

    EXPECT_EQ(6, children.size());

    vector<FString>::const_iterator end(children.end());

    EXPECT_TRUE(find(children.begin(), children.end(), "file0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "file1") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "file2") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "dir0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "dir1") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "dir2") != end);
}

TEST_F(FileSystemImplOnBoxTest, GetChildren_Recurse_YesPaths_YesDirNames)
{
    FileSystemImpl f;

    f.MakeDir("/tmp/root");
    f.Touch("/tmp/root/file0");
    f.Touch("/tmp/root/file1");
    f.MakeDir("/tmp/root/dir0");
    f.MakeDir("/tmp/root/dir1");
    f.Touch("/tmp/root/dir0/file2");
    f.MakeDir("/tmp/root/dir0/dir2");

    vector<FString> children;
    f.GetChildren("/tmp/root", children, true, true, true);

    EXPECT_EQ(6, children.size());

    vector<FString>::const_iterator end(children.end());

    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/file0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/file1") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/dir0/file2") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/dir0") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/dir1") != end);
    EXPECT_TRUE(find(children.begin(), children.end(), "/tmp/root/dir0/dir2") != end);
}
