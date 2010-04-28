
#include "boost/test/unit_test.hpp"
#include "LogManager.h"
#include "Context.h"
#include "FileSystem.h"
#include "ProcFileSystem.h"
#include "MockFileSystem.h"

using namespace boost;
using namespace boost::unit_test;
using namespace Forte;

LogManager logManager;

void contextGetObject(const Forte::Context &context)
{
    ObjectPtr ob;
    ob = context.Get("objecttest");
    ob = context.Get<Object>("objecttest");
}


void contextGetFileSystem(const Forte::Context &context)
{
    shared_ptr<FileSystem> fsptr;
    
    fsptr = context.Get<FileSystem>("filesystemtest");
    fsptr->GetCWD();
}

BOOST_AUTO_TEST_CASE(ContextGetInvalidKey)
{
    hlog(HLOG_INFO, "ContextGetInvalidKey");
    Context c;
    BOOST_CHECK_THROW(contextGetObject(c), EInvalidKey);
}

BOOST_AUTO_TEST_CASE(ContextSetGetObject)
{
    hlog(HLOG_INFO, "ContextSetGetObject");
    Context c;
    ObjectPtr ptr(new Object());
    c.Set("objecttest", ptr);
    contextGetObject(c);
}

BOOST_AUTO_TEST_CASE(ContextSetGetFileSystem)
{
    hlog(HLOG_INFO, "ContextSetGetFileSystem");
    Context c;
    shared_ptr<FileSystem> fsptr(new FileSystem());
    c.Set("filesystemtest", fsptr);
    contextGetFileSystem(c);
}

BOOST_AUTO_TEST_CASE(ContextSetObjectGetFileSystem)
{
    hlog(HLOG_INFO, "ContextSetObjectGetFileSystem");
    Context c;
    ObjectPtr ptr(new Object());
    c.Set("filesystemtest", ptr);
    BOOST_CHECK_THROW(contextGetFileSystem(c), EContextTypeMismatch)
}

BOOST_AUTO_TEST_CASE(UptimeMockContents)
{
    hlog(HLOG_INFO, "UptimeMockContents");
    Context c;
    // setup
    shared_ptr<MockFileSystem> fsptr(new MockFileSystem());
    fsptr->FilePutContents("/proc/uptime", "30782.38 29768.69\n");

    c.Set("FileSystem", fsptr);

    // construct
    ProcFileSystem procFileSystem(c);

    // test uptime
    ProcFileSystem::Uptime uptime;
    procFileSystem.UptimeRead(uptime);
    
    BOOST_CHECK_EQUAL(uptime.mSecondsUp, 30782.38);
    BOOST_CHECK_EQUAL(uptime.mSecondsIdle, 29768.69);
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

