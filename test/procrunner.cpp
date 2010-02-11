#include "Forte.h"
#include "ansi_test.h"

// functions
void* testTimeout(void* p)
{

    return NULL;
}


// main
int main(int argc, char *argv[])
{
    bool all_pass = true;
    CLogManager logManager;
    logManager.BeginLogging();
    logManager.SetGlobalLogMask(HLOG_ALL);

    ProcRunner& runner = ProcRunner::getRef();
    FString output;
    
    runner.run("./2minuteprocess", "", &output, 10);
    
    if (FileSystem::get()->file_exists("./2minutetouchfile"))
    {
	all_pass = false;
	hlog(HLOG_WARN, "timeout failed");
    }

    runner.run("./2secondprocess", "", &output, 4);
    
    if (!FileSystem::get()->file_exists("./2secondtouchfile"))
    {
	all_pass = false;
	hlog(HLOG_WARN, "timeout did not wait long enough");
    }
    else
    {
	FileSystem::get()->unlink("./2secondtouchfile");
    }
    // done
    return (all_pass ? 0 : 1);
}

