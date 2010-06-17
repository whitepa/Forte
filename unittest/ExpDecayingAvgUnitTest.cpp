#define BOOST_TEST_MODULE "ExpDecayingAvg Unit Tests"

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include "LogManager.h"
#include "Context.h"
#include "ExpDecayingAvg.h"
#include "RunLoop.h"

using namespace boost;
using namespace boost::unit_test;
using namespace Forte;

LogManager logManager;

BOOST_AUTO_TEST_CASE(Test1)
{
    logManager.BeginLogging();
    // setup
    Context c;
    c.Set("forte.RunLoop", make_shared<RunLoop>());
    
    ExpDecayingAvg a(c, 1000);
    a.Set(5.0);

    while (1)
    {
        hlog(HLOG_INFO, "value is %f", a.Get(1000));
        usleep(250000);
    }
}
