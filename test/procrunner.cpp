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
    LogManager logManager;
    logManager.BeginLogging();
    logManager.SetGlobalLogMask(HLOG_ALL);

    ProcRunner runner;
    FileSystem fs;
    FString output;
    
    runner.Run("./2minuteprocess", "", &output, 10);
    
    if (fs.FileExists("./2minuteTouchfile"))
    {
        all_pass = false;
        hlog(HLOG_WARN, "timeout failed");
    }

    runner.Run("./2secondprocess", "", &output, 4);
    
    if (!fs.FileExists("./2secondTouchfile"))
    {
        all_pass = false;
        hlog(HLOG_WARN, "timeout did not wait long enough");
    }
    else
    {
        fs.Unlink("./2secondTouchfile");
    }
    // done
    return (all_pass ? 0 : 1);
}

