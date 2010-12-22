#include <gtest/gtest.h>
#include "LogManager.h"
#include "MockFileSystem.h"
#include "FTrace.h"

using namespace Forte;

class MockFileSystemUnitTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        mLogManager = new LogManager();
        mLogManager->BeginLogging("MockFileSystemUnitTest.log");
        hlog(HLOG_DEBUG, "Begin MockFileSystem unit test");

    }

    static void TearDownTestCase() {
        delete mLogManager;

    }

    static LogManager *mLogManager;

};

LogManager * MockFileSystemUnitTest::mLogManager = NULL;


TEST_F(MockFileSystemUnitTest, AddDirectoryPathToFileSystem)
{
    FTRACE;

    MockFileSystem mockFS;
    mockFS.AddDirectoryPathToFileSystem("/foo/1/2/3");
    mockFS.AddDirectoryPathToFileSystem("/bar/3/4/5");


    ASSERT_TRUE(mockFS.FileExists("/foo"));
    ASSERT_TRUE(mockFS.FileExists("/foo/1"));
    ASSERT_TRUE(mockFS.FileExists("/foo/1/2"));
    ASSERT_TRUE(mockFS.FileExists("/foo/1/2/3"));

    ASSERT_TRUE(mockFS.FileExists("/bar"));
	ASSERT_TRUE(mockFS.FileExists("/bar/3"));
	ASSERT_TRUE(mockFS.FileExists("/bar/3/4"));
	ASSERT_TRUE(mockFS.FileExists("/bar/3/4/5"));


	struct stat st;
	ASSERT_TRUE(mockFS.Stat("/foo", &st));
	ASSERT_TRUE(mockFS.IsDir("/foo"));
	ASSERT_TRUE(mockFS.Stat("/foo/1", &st));
	ASSERT_TRUE(mockFS.IsDir("/foo/1"));
	ASSERT_TRUE(mockFS.Stat("/foo/1/2", &st));
	ASSERT_TRUE(mockFS.IsDir("/foo/1/2"));
	ASSERT_TRUE(mockFS.Stat("/foo/1/2/3", &st));
	ASSERT_TRUE(mockFS.IsDir("/foo/1/2/3"));

	ASSERT_TRUE(mockFS.Stat("/bar", &st));
	ASSERT_TRUE(mockFS.IsDir("/bar"));
	ASSERT_TRUE(mockFS.Stat("/bar/3", &st));
	ASSERT_TRUE(mockFS.IsDir("/bar/3"));
	ASSERT_TRUE(mockFS.Stat("/bar/3/4", &st));
	ASSERT_TRUE(mockFS.IsDir("/bar/3/4"));
	ASSERT_TRUE(mockFS.Stat("/bar/3/4/5", &st));
	ASSERT_TRUE(mockFS.IsDir("/bar/3/4/5"));


	struct dirent **namelist;

	ASSERT_EQ(2, mockFS.ScanDir("/", &namelist, alphasort));

	ASSERT_EQ(1, mockFS.ScanDir("/foo", &namelist, alphasort));
	FString nm(namelist[0]->d_name);
	ASSERT_EQ("1",nm);

	ASSERT_EQ(1, mockFS.ScanDir("/foo/1", &namelist, alphasort));
	FString nm2(namelist[0]->d_name);
	ASSERT_EQ("2",nm2);

	ASSERT_EQ(1, mockFS.ScanDir("/foo/1/2", &namelist, alphasort));
	FString nm3(namelist[0]->d_name);
	ASSERT_EQ("3",nm3);

	ASSERT_EQ(0, mockFS.ScanDir("/foo/1/2/3", &namelist, alphasort));

	ASSERT_EQ(1, mockFS.ScanDir("/bar", &namelist, alphasort));
	FString nm4(namelist[0]->d_name);
	ASSERT_EQ("3",nm4);

	ASSERT_EQ(1, mockFS.ScanDir("/bar/3", &namelist, alphasort));
	FString nm5(namelist[0]->d_name);
	ASSERT_EQ("4",nm5);

	ASSERT_EQ(1, mockFS.ScanDir("/bar/3/4", &namelist, alphasort));
	FString nm6(namelist[0]->d_name);
	ASSERT_EQ("5",nm6);

	ASSERT_EQ(0, mockFS.ScanDir("/bar/3/4/5", &namelist, alphasort));



}



