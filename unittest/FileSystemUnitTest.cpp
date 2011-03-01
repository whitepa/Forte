
#include "boost/test/unit_test.hpp"
#include "LogManager.h"
#include "FileSystem.h"
#include "SystemCallUtil.h"

using namespace boost::unit_test;
using namespace Forte;

LogManager logManager;

BOOST_AUTO_TEST_CASE(StatFS)
{
    hlog(HLOG_INFO, "StatFS");
    FileSystem f;
    struct statfs st;
    f.StatFS("/", &st);
}

BOOST_AUTO_TEST_CASE(StatFSPathDoesNotExist)
{
    hlog(HLOG_INFO, "StatFSPathDoesNotExist");
    FileSystem f;
    struct statfs st;
    BOOST_CHECK_THROW(f.StatFS("pathdoesnotexist", &st), EErrNoENOENT);
}

BOOST_AUTO_TEST_CASE(ScanDirReplacement)
{
    hlog(HLOG_INFO, "ScanDirReplacement");
    FileSystem f;

    vector<FString> names;
    f.ScanDir("/",names);
    BOOST_CHECK(names.size() != 0);
}

BOOST_AUTO_TEST_CASE(TouchFileAndTestTimes)
{
    hlog(HLOG_INFO, "TouchFileAndTestTimes");
    FileSystem f;
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
        BOOST_CHECK(abs(st.st_atime - currTime) < 2);
        BOOST_CHECK(abs(st.st_mtime - currTime) < 2);
    }
}



////Boost Unit init function ///////////////////////////////////////////////////
test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    logManager.SetLogMask("//stdout", HLOG_ALL);
    logManager.BeginLogging("//stdout");

    // initialize everything here

    return 0;
}
////////////////////////////////////////////////////////////////////////////////
