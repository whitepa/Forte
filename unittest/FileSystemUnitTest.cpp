
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
