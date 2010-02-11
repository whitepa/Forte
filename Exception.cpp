// Exception.cpp
#include "Types.h"
#include "Exception.h"
#include "FTrace.h"
#include "LogManager.h"
#include "ProcRunner.h"
#include <sys/types.h>
#include <unistd.h>

CException::CException()
{
    FTrace::getStack(mStack);
}

CException::CException(const char *description) :
    mDescription(description)
{
    FTrace::getStack(mStack);
}
CException::CException(const FStringFC &fc, const char *format, ...)
{
    va_list ap;
    int size;
    // get size
    va_start(ap, format);
    size = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    va_start(ap, format);
    mDescription.vFormat(format, size, ap);
    va_end(ap);

    FTrace::getStack(mStack);
}
CException::CException(const CException& other) :
    mDescription(other.mDescription),
    mStack(other.mStack)
{
}
CException::~CException() throw()
{
}

std::string CException::extendedDescription()
{
    FString stack;
    FTrace::formatStack(mStack, stack);
    return FString(FStringFC(),"%s; STACK: %s", mDescription.c_str(), stack.c_str());
}


void CException::pretty_trace_log(int log_level)
{
    std::list<void*>::const_iterator si;
    std::vector<FString>::iterator li;
    std::vector<FString> trace, mapping;
    FString cmd, output;
    int rc;

    // start exception stack iteration
    si = mStack.begin();

    // have stack?
    if (si != mStack.end())
    {
        // generate stack trace
        hlog(log_level, "Generating stack trace...");

        // get /proc/<pid>/maps
        cmd.Format("cat /proc/%u/maps | awk '{print $1\" \"$6}'", getpid());
        output = ProcRunner::get()->read_pipe(cmd, &rc);
        output.lineSplit(mapping, true);

        if (rc != 0)
        {
            hlog(log_level, "<no map available>");
            return;
        }

        // generate stack trace
        for (++si /*skip first frame*/; si != mStack.end(); ++si)
        {
            trace.push_back(pretty_frame(mapping, *si));
        }

        // log trace
        hlog(log_level, "Stack Trace:");

        for (li = trace.begin(); li != trace.end(); ++li)
        {
            hlog(log_level, *li);
        }
    }
}


FString CException::pretty_frame(const std::vector<FString>& mapping, void *address)
{
    ProcRunner *proc = ProcRunner::get();
    FString stmp, cmd, mod, name, func, file, output;
    std::vector<FString>::const_iterator li;
    std::vector<FString> lines, parts;
    unsigned long long a = (unsigned long long)address, r1, r2;
    int err;

    // find module for the current address using map
    for (li = mapping.begin(); li != mapping.end(); ++li)
    {
        parts = li->split(" ", 2);
        if (parts.size() < 2) continue;
        file = parts[1];
        parts = parts[0].split("-");
        if (parts.size() < 2) continue;
        r1 = strtoull(parts[0].c_str(), NULL, 16);
        r2 = strtoull(parts[1].c_str(), NULL, 16);

        if (r1 <= a && a < r2)
        {
            mod = file.Trim();
            break;
        }
    }

    // verify mapping
    if (mod.empty())
    {
        stmp.Format("%#018llx <unknown mapping>", (u64)address);
        return stmp;
    }

    // find file name and line number of given address
    name = mod.Mid(mod.find_last_of('/') + 1);
    mod = proc->shell_escape(mod);
    cmd.Format("/usr/bin/addr2line -C -f -e %s %#18llx", mod.c_str(), (u64)address);
    output = proc->read_pipe(cmd, &err);

    if (err != 0)
    {
        stmp.Format("%#018llx [%s] <invalid library>", (u64)address, name.c_str());
        return stmp;
    }

    // parse output
    output.Trim().lineSplit(lines, true);

    if (lines.size() < 2)
    {
        stmp.Format("%#018llx [%s] %s", (u64)address, name.c_str(), output.c_str());
        return stmp;
    }

    // log frame
    func = lines[0].Trim();
    size_t pos = func.find('(') + 1;
    if (pos) func = func.Left(pos) + ")";
    else func += "()";
    file = lines[1].Trim();
    file = file.Mid(file.find_last_of('/') + 1);
    stmp.Format("%#018llx [%s] %-30s %s",
                (u64)address, name.c_str(), file.c_str(), func.c_str());
    return stmp;
}
