#define BOOST_TEST_MODULE "RandomGenerator Unit Tests"

#include "boost/test/unit_test.hpp"
#include <boost/shared_ptr.hpp>

#include "LogManager.h"
#include "Context.h"
#include "RandomGenerator.h"
#include "Foreach.h"

using namespace boost::unit_test;
using namespace boost;
using namespace Forte;

LogManager logManager;

BOOST_AUTO_TEST_CASE(RandomGeneratorTest)
{
    hlog(HLOG_INFO, "RandomGenerator");
    
    RandomGenerator rg;
    FString data;
    rg.GetRandomData(100, data);

    BOOST_CHECK_EQUAL(data.size(), 100UL);

    unsigned int a = rg.GetRandomUInt();
    hlog(HLOG_DEBUG, "random uint is %u", a);
}

