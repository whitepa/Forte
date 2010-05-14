// test program for the forte library

#include "Forte.h"
#include "UtilTest.h"

using namespace Forte;

int main2(int argc, char *argv[]);
int main(int argc, char *argv[])
{
    int ret = -1;

    try
    {
        ret = main2(argc, argv);
    }
    catch (FString& err)
    {
        cerr << "ABORT: " << err << endl;
    }
    catch (const char *err)
    {
        cerr << "ABORT: " << err << endl;
    }
    catch (exception& err)
    {
        cerr << "ABORT: " << err.What() << endl;
    }
    catch (Exception &e)
    {
        cerr << "ABORT: " << e.GetDescription() << endl;
    }
    return ret;
}

int main2(int argc, char *argv[])
{
    // start logging to stderr
    char *fake_argv[] = { "utiltest", "-d", "-v" };
    ServerMain main(sizeof(fake_argv)/sizeof(char*), fake_argv, "c:dv", "");
    LogManager& lm = main.mLogManager;
    lm.BeginLogging();
    lm.BeginLogging("//syslog/utiltest");

    hlog(HLOG_INFO, "Forte Utilities test program");

    // log to a log file too
    lm.BeginLogging("test_output.log");
    hlog(HLOG_INFO, "First line of output in test_output.log");
    lm.EndLogging("test_output.log");
    hlog(HLOG_INFO, "This line should not appear in test_output.log");
    lm.BeginLogging("test_output.log");

    hlog(HLOG_DEBUG, "Testing debug logging (1/5)");
    hlog(HLOG_CRIT, "Testing critical logging (2/5)");
    hlog(HLOG_ERR, "Testing error logging (3/5)");
    hlog(HLOG_WARN, "Testing warning logging (4/5)");
    hlog(HLOG_INFO, "Testing informational logging (5/5)");

    hlog(HLOG_INFO, "Creating thread pool dispatcher");

    /*
    // create a thread pool dispatcher
    TestRequestHandler rh;
    ThreadPoolDispatcher disp(rh,  // request handler
                               5,   // min threads
                               20,  // max threads
                               5,   // min spare threads
                               10,  // max spare threads
                               100, // deep queue threshold
                               40,  // max queue depth
                               "test-pool"); // dispatcher name
    sleep(2);

    // throw some events at it
    for (int i = 0; i < 1; i++)
    {
        TestEvent *e = new TestEvent();
        e->mStr.Format("event #%d", i);
        disp.Enqueue(e);
    }

    sleep(20);
    */
    return 0;
}

TestEvent::TestEvent()
{
    hlog(HLOG_INFO, "event ctor");
}
TestEvent::~TestEvent()
{
    hlog(HLOG_INFO, "event dtor");
}
void TestRequestHandler::handler(Event *e)
{
    TestEvent *te = dynamic_cast<TestEvent*>(e);
    if (te == NULL)
    {
        FString err;
        err.Format("dynamic cast failed in handler()");
        throw ForteUtilTestException(err);
    }
    hlog(HLOG_INFO, "running handler(); str is %s", te->mStr.c_str());
    sleep(1);
}

void TestRequestHandler::busy()
{}
void TestRequestHandler::periodic()
{
    hlog(HLOG_INFO, "running periodic()");
}
void TestRequestHandler::init()
{
    hlog(HLOG_INFO, "running init()");
}
void TestRequestHandler::cleanup()
{
    hlog(HLOG_INFO, "running cleanup()");
}
