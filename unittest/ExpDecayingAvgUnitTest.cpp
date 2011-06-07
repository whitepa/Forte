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

BOOST_AUTO_TEST_CASE(CheckDeadLock)
{
    logManager.BeginLogging();

    // setup
    RunLoop rl;
    sleep(2);    
    BOOST_CHECK(rl.IsEmpty());
}

BOOST_AUTO_TEST_CASE(Test1)
{
    logManager.BeginLogging();
    // setup
    {
        ExpDecayingAvg a(make_shared<RunLoop>(), EDA_VALUE, 10000);

        for (int x = 0; x < 30; x++)
        {
            a.Set(x * 5.0);
        
            hlog(HLOG_INFO, "value is %f  rate is %f", a.Get(), a.GetRate(1000));
            sleep(1);
        }
        for (int x = 30; x > 0; x--)
        {
            a.Set(x * 5.0);
        
            hlog(HLOG_INFO, "value is %f  rate is %f", a.Get(), a.GetRate(1000));
            sleep(1);
        }

    }
    sleep(1);
}

