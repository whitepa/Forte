
#include "boost/test/unit_test.hpp"
#include "LogManager.h"
#include "Context.h"
#include "FileSystem.h"

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
    hlog(HLOG_INFO, "%s()", __FUNCTION__);
    Context c;
    BOOST_CHECK_THROW(contextGetObject(c), EInvalidKey);
}

BOOST_AUTO_TEST_CASE(ContextSetGetObject)
{
    hlog(HLOG_INFO, "%s()", __FUNCTION__);
    Context c;
    ObjectPtr ptr(new Object());
    c.Set("objecttest", ptr);
    contextGetObject(c);
}

BOOST_AUTO_TEST_CASE(ContextSetGetFileSystem)
{
    hlog(HLOG_INFO, "%s()", __FUNCTION__);
    Context c;
    shared_ptr<FileSystem> fsptr(new FileSystem());
    c.Set("filesystemtest", fsptr);
    contextGetFileSystem(c);
}

BOOST_AUTO_TEST_CASE(ContextSetObjectGetFileSystem)
{
    hlog(HLOG_INFO, "%s()", __FUNCTION__);
    Context c;
    ObjectPtr ptr(new Object());
    c.Set("filesystemtest", ptr);
    BOOST_CHECK_THROW(contextGetFileSystem(c), EContextTypeMismatch)
}


/*BOOST_AUTO_TEST_CASE(ContextSetGetInt)
{
    hlog(HLOG_INFO, "%s()", __FUNCTION__);
    Context c;
    ObjectPtr ptr(new int());
    c.Set("intptr", ptr);
    contextGetInt(c);
    }*/


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

