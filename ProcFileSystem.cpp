
#include "ProcFileSystem.h"
#include "FileSystem.h"

#include <boost/regex.hpp>

using namespace boost;
using namespace Forte;

ProcFileSystem::ProcFileSystem(const Context& ctxt)
    : mContext(ctxt)
{
}

ProcFileSystem::~ProcFileSystem()
{
}

void ProcFileSystem::UptimeRead(Uptime& uptime)
{
    // open /proc/uptime
    // read information in the form of "[seconds up] [seconds idle]"
    // return info to user
    FileSystemPtr fsptr;
    fsptr = mContext.Get<FileSystem>("FileSystem");

    FString output;
    output = fsptr->FileGetContents("/proc/uptime").Trim();

    regex e("(.*) (.*)");
    cmatch m;

    FString secondsUp, secondsIdle;

    if (regex_match(output.c_str(), m, e))
    {
        secondsUp.assign(m[1].first, m[1].second);
        secondsIdle.assign(m[2].first, m[2].second);
        hlog(HLOG_DEBUG4, "found %s -> %s", 
             secondsUp.c_str(), secondsIdle.c_str());

        uptime.mSecondsUp = secondsUp.AsDouble();
        uptime.mSecondsIdle = secondsIdle.AsDouble();
    }
    else
    {
        throw EProcFileSystem("Invalid value from /proc/uptime");
    }
    
    
}
