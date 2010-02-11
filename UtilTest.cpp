// test program for the forte library

#include "Forte.h"
#include "UtilTest.h"

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
        cerr << "ABORT: " << err.what() << endl;
    }
    catch (CException &e)
    {
        cerr << "ABORT: " << e.getDescription() << endl;
    }
    return ret;
}

int main2(int argc, char *argv[])
{
    // start logging to stderr
    char *fake_argv[] = { "utiltest", "-d", "-v" };
    CServerMain main(sizeof(fake_argv)/sizeof(char*), fake_argv, "c:dv", "");
    CLogManager& lm = main.mLogManager;
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
    CTestRequestHandler rh;
    CThreadPoolDispatcher disp(rh,  // request handler
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
        CTestEvent *e = new CTestEvent();
        e->mStr.Format("event #%d", i);
        disp.enqueue(e);
    }

    sleep(20);
    */
    return 0;
}

CTestEvent::CTestEvent()
{
    hlog(HLOG_INFO, "event ctor");
}
CTestEvent::~CTestEvent()
{
    hlog(HLOG_INFO, "event dtor");
}
void CTestRequestHandler::handler(CEvent *e)
{
    CTestEvent *te = dynamic_cast<CTestEvent*>(e);
    if (te == NULL)
    {
        FString err;
        err.Format("dynamic cast failed in handler()");
        throw CForteUnilTestException(err);
    }
    hlog(HLOG_INFO, "running handler(); str is %s", te->mStr.c_str());
    sleep(1);
}

void CTestRequestHandler::busy()
{}
void CTestRequestHandler::periodic()
{
    hlog(HLOG_INFO, "running periodic()");
}
void CTestRequestHandler::init()
{
    hlog(HLOG_INFO, "running init()");
}
void CTestRequestHandler::cleanup()
{
    hlog(HLOG_INFO, "running cleanup()");
}
