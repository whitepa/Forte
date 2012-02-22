#include "ProcFileSystem.h"
#include "FileSystem.h"
#include "Foreach.h"
#include "FTrace.h"
#include "LogManager.h"
#include <boost/regex.hpp>
#include <stdio.h>
#include <errno.h>

using namespace std;
using namespace boost;
using namespace Forte;

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

unsigned int ProcFileSystem::CountOpenFileDescriptors(pid_t pid)
{
    Forte::FStringVector children;
    mFileSystemPtr->GetChildren("/proc/" + FString(pid) + "/fd", children);

    return children.size();
}

void ProcFileSystem::PidOf(const FString& runningProg, std::vector<pid_t>& pids) const
{
    FTRACE2("%s", runningProg.c_str());

    Forte::FStringVector children;
    mFileSystemPtr->ScanDir("/proc", children);
    FString cmdlinePath, cmdline, programName, pid;

    foreach (const FString& child, children)
    {
        cmdlinePath = "/proc/" + child + "/cmdline";
        pid = mFileSystemPtr->Basename(child);

        if (pid.IsNumeric() && mFileSystemPtr->FileExists(cmdlinePath))
        {
            cmdline = mFileSystemPtr->FileGetContents(cmdlinePath).c_str();
            programName = mFileSystemPtr->Basename(cmdline);

            if (programName == runningProg)
            {
                hlog(HLOG_DEBUG, "found match");
                pids.push_back((pid_t) pid.AsInteger());
            }
        }
    }
    if (pids.empty())
    {
        hlog_and_throw(HLOG_DEBUG, EProcFileSystemProcessNotFound());
    }
}

bool ProcFileSystem::ProcessIsRunning(const FString& runningProg) const
{
    try
    {
        vector<pid_t> pids;
        PidOf(runningProg, pids);
        return true;
    }
    catch (EProcFileSystemProcessNotFound &e)
    {
        return false;
    }
}

void ProcFileSystem::SetOOMScore(pid_t pid, const FString &score)
{
    FString procFileName(FStringFC(), "/proc/%d/oom_score_adj", pid);
    hlog(LOG_DEBUG,"opening proc file %s", procFileName.c_str());

    AutoFD  fd;
    char errmsg[256];
    char const *errstr;
    if ((fd = ::open(procFileName.c_str(), O_WRONLY)) < 0)
    {
        errstr = strerror_r(errno, errmsg, 256);
        hlog(LOG_ERR,"error opening %s, error :%s",
             procFileName.c_str(), errstr);

        throw EProcFileSystem(errstr);
    }
    if ((size_t)::write(fd, score.c_str(), score.length()) != score.length())
    {
        errstr = strerror_r(errno, errmsg, 256);
        hlog(LOG_ERR,"error writing %s, error :%s",
             procFileName.c_str(),errstr);

        throw EProcFileSystem(errstr);
    }

    hlog(LOG_DEBUG, "Sucessfully changed the oom score of %d process",
         pid);
}

////////////////////////////////////////////////////////////////////////////////
// helpers
////////////////////////////////////////////////////////////////////////////////

void ProcFileSystem::getProcFileContents(const FString& pathInSlashProc,
                                         FString& contents)
{
    contents = mFileSystemPtr->FileGetContents("/proc/" + pathInSlashProc).Trim();
}
