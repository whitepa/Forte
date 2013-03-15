#include "Forte.h"
#include "ansi_test.h"

// functions
void* test(void* p)
{

    return NULL;
}


// main
int main(int argc, char *argv[])
{
    bool all_pass = true;
    LogManager logManager;
    logManager.BeginLogging();
    logManager.SetGlobalLogMask(HLOG_ALL);

    ServiceConfig config;
    FString file = "./sampleserviceconfig";
    printf("Loading config, should see a warning\n");
    config.ReadConfigFile(file);
    config.Display();

    if (config.Get("validvalue1") != "1")
    {
	all_pass = false;
	hlog(HLOG_WARN, "missing validvalue1");
    }

    if (config.GetInteger("validvalue1") != 1)
    {
	all_pass = false;
	hlog(HLOG_WARN, "missing validvalue1 as integer");
    }

    if (config.Get("validvalue2") != "2")
    {
	all_pass = false;
	hlog(HLOG_WARN, "missing validvalue2");
    }

    if (config.Get("validvalue3") != "3")
    {
	all_pass = false;
	hlog(HLOG_WARN, "missing validvalue2");
    }

    if (config.Get("validvalue4") != "this is a valid value")
    {
	all_pass = false;
	hlog(HLOG_WARN, "missing validvalue4");
    }

    if (config.Get("validvalue5") != "\"this value should be quoted\"")
    {
	all_pass = false;
	hlog(HLOG_WARN, "missing validvalue4");
    }

    if (config.Get("invalidvalue1") != "")
    {
	all_pass = false;
	hlog(HLOG_WARN, "have value for invalidvalue1");
    }

    if (config.Get("invalidvalue2") != "")
    {
	all_pass = false;
	hlog(HLOG_WARN, "have value for invalidvalue2");
    }

    // done
    return (all_pass ? 0 : 1);
}

