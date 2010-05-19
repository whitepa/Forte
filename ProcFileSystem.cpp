
#include "ProcFileSystem.h"
#include "FileSystem.h"
#include "Foreach.h"

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
    FString contents;
    getProcFileContents("uptime", contents);

    regex e("(.*) (.*)");
    cmatch m;

    FString secondsUp, secondsIdle;

    if (regex_match(contents.c_str(), m, e))
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

void ProcFileSystem::MemoryInfoRead(Forte::StrDoubleMap& meminfo)
{
    FString contents;
    getProcFileContents("meminfo", contents);

    std::vector<FString> lines;
    contents.LineSplit(lines, true);
    foreach (const FString &line, lines)
    {
        std::vector<FString> components;
        line.Explode(":", components, true);
        if (line.empty()) continue;
        std::vector<FString> vals;
        if (components.size() != 2)
        {
            hlog(LOG_ERR, "bad parse from /proc/meminfo; data is '%s'",
                 line.c_str());
            continue;
        }

        // found two types of output so far:
        //"VmallocChunk: 34359469547 kB\n"
        //"HugePages_Total:     0\n"
        // haven't found a situation where HugePages output puts

        components[1].Explode(" ", vals, true);
        if (vals.size() != 2)
        {
            if (components[1].Trim() == "0")
            {
                // handling HugePages_Total:     0
                hlog(LOG_DEBUG, 
                     "non standard parse from /proc/meminfo; mem value is '%s'",
                     components[1].c_str());
            }
            else
            {
                hlog(LOG_ERR, "bad parse from /proc/meminfo; mem value is '%s'",
                     components[1].c_str());
                continue;                
            }
        }

        if (!vals[0].IsNumeric())
        {
            hlog(LOG_ERR, "bad parse from /proc/meminfo; "
                 "mem value is not numeric: '%s'",
                 vals[0].c_str());
            continue;
        }

        meminfo[components[0]] = vals[0].AsDouble();
    }
}







////////////////////////////////////////////////////////////////////////////////
// helpers
////////////////////////////////////////////////////////////////////////////////

void ProcFileSystem::getProcFileContents(const FString& pathInSlashProc, 
                                         FString& contents)
{
    FileSystemPtr fsptr;
    fsptr = mContext.Get<FileSystem>("forte.FileSystem");
    contents = fsptr->FileGetContents("/proc/" + pathInSlashProc).Trim();
}
