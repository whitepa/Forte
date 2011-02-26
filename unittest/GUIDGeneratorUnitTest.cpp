#define BOOST_TEST_MODULE "GUIDGenerator Unit Tests"

#include "boost/test/unit_test.hpp"
#include <boost/shared_ptr.hpp>

#include "LogManager.h"
#include "Context.h"
#include "GUIDGenerator.h"
#include "Foreach.h"

using namespace boost::unit_test;
using namespace boost;
using namespace Forte;

LogManager logManager;

BOOST_AUTO_TEST_CASE(GUIDGeneratorTest)
{
    logManager.BeginLogging("//stderr");
    hlog(HLOG_INFO, "GUIDGenerator, verifying 1M unique GUIDs");
    
    GUIDGenerator gg;
    std::set<FString> guids;
    unsigned int num = 1000000;
    unsigned int i = num;
    while (i--)
    {
        FString guid;
        gg.GenerateGUID(guid);
        guids.insert(guid);
    }
    BOOST_CHECK(guids.size() == num);
}

