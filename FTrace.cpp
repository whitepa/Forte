/// On-demand Backtrace and Profiling capabilities
///
/// NOTE:  the 'totalSpent' time from profiling currently includes time spent
///        within the profiling code as well.  'spent' does not.  This should
///        be fixed eventually...

#include "Forte.h"

#include <dlfcn.h>

ThreadKey FTrace::sTraceKey(FTrace::Cleanup);
unsigned int FTrace::sInitialized = 0;

////////////////////////////////////////////////////////////
// GCC Instrumentation hooks:
extern "C" {
    void __cyg_profile_func_enter(void *this_fn, void *call_site)
        __attribute__ ((no_instrument_function));
    void __cyg_profile_func_enter(void *this_fn, void *call_site)
    {
        if (FTrace::sInitialized == 0xdeadbeef)
            FTrace::Enter(this_fn, call_site);
    }
    void __cyg_profile_func_exit(void *this_fn, void *call_site)
        __attribute__ ((no_instrument_function));
    void __cyg_profile_func_exit(void *this_fn, void *call_site)
    {
        if (FTrace::sInitialized == 0xdeadbeef)
            FTrace::Exit(this_fn, call_site);
    }
}
////////////////////////////////////////////////////////////
FProfileData::FProfileData(void *fn, const struct timeval &spent, const struct timeval &totalSpent) :
        mFunction(fn),mCalls(1),mSpent(spent),mTotalSpent(totalSpent)
    {}
void FProfileData::AddCall(const struct timeval &spent, const struct timeval &totalSpent)
{
    mSpent = mSpent + spent;
    mTotalSpent = mTotalSpent + totalSpent;
    ++mCalls;
    if (mCalls % 10000 == 0)
    {
        // 10k calls to this function
        // take a stack sample
        FTrace::GetStack(mStackSample);
    }
}


FTraceThreadInfo::FTraceThreadInfo() :
    depth(0),profiling(false),fn(NULL)
{

}
FTraceThreadInfo::~FTraceThreadInfo()
{
    typedef pair<void *,FProfileData *> bob;
    foreach(const bob p,profileData)
        delete p.second;
    profileData.clear();
}
FTraceThreadInfo* FTrace::getThreadInfo(void)
{
    FTraceThreadInfo *i = reinterpret_cast<FTraceThreadInfo *>(sTraceKey.Get());
    if (i == (void*)1) return NULL;  // (void*)1 means we hit a recursion loop
    if (i == NULL)
    {
        sTraceKey.Set((void*)1);
        i = new FTraceThreadInfo();
        sTraceKey.Set(i);
    }
    return i;
}
void FTraceThreadInfo::StoreProfile(void *fn, struct timeval &spent, struct timeval &totalSpent)
{
    if (profileData.find(fn) == profileData.end())
        profileData[fn] = new FProfileData(fn, spent, totalSpent);
    else
        profileData[fn]->AddCall(spent, totalSpent);
}

void FTrace::Enable()
{
    getThreadInfo();
    sInitialized = 0xdeadbeef;
}

void FTrace::Disable()
{
    sInitialized = 0;
}

void FTrace::Enter(void *fn, void *caller)
{
    FTraceThreadInfo *info = getThreadInfo();
    if (info == NULL) return;  // avoid infinite loops!
    FTraceThreadInfo &i(*info);
    unsigned short depth = i.depth++;
    // 'push' to the stack
    if (depth < FTRACE_MAXDEPTH)
    {
        // trace data
        i.stack[depth] = caller;
        i.lastfn[depth] = i.fn;
        i.fn = fn;
        if (i.profiling)
        {
            // profile data
            gettimeofday(&(i.entry[depth]),0);
            i.reentry[depth].tv_sec = 0;
            i.reentry[depth].tv_usec = 0;
            i.spent[depth].tv_sec = 0;
            i.spent[depth].tv_usec = 0;
            if (depth > 0)
                // update spent time on the caller
                if (i.reentry[depth-1].tv_sec == 0 && i.reentry[depth-1].tv_usec == 0)
                    i.spent[depth-1] = i.entry[depth] - i.entry[depth-1];
                else
                    i.spent[depth-1] = i.spent[depth-1] + (i.entry[depth] - i.reentry[depth-1]);
        }
    }
}
void FTrace::Exit(void *fn, void *caller)
{
    FTraceThreadInfo *info = getThreadInfo();
    if (info == NULL) return;  // avoid infinite loops!
    FTraceThreadInfo &i(*info);
    if (i.depth)
    {
        // 'pop' from the stack
        unsigned short depth = --i.depth;
        if (i.profiling)
        {
            struct timeval now;
            gettimeofday(&now, 0);
            if (i.reentry[depth].tv_sec == 0 && i.reentry[depth].tv_usec == 0)
                i.spent[depth] = now - i.entry[depth];
            else // we called another function
                i.spent[depth] = i.spent[depth] + (now - i.reentry[depth]);
            if (depth > 1)
                i.reentry[depth-1] = now;
            // record profile info
            struct timeval totalSpent = now - i.entry[depth];
            i.StoreProfile(i.fn, i.spent[depth], totalSpent);
            i.fn = i.lastfn[depth];
        }
    }
}
void FTrace::Cleanup(void *ptr)
{
    FTraceThreadInfo *i = reinterpret_cast<FTraceThreadInfo *>(ptr);
    if (i != NULL) delete i;
}
unsigned int FTrace::GetDepth(void)
{
    FTraceThreadInfo *info = getThreadInfo();
    if (info == NULL) return 0;  // avoid infinite loops!
    FTraceThreadInfo &i(*info);
    return i.depth;
}
void FTrace::SetProfiling(bool profile)
{
    FTraceThreadInfo *info = getThreadInfo();
    if (info == NULL) return;  // avoid infinite loops!
    FTraceThreadInfo &i(*info);
    i.profiling = profile;
    if (profile)
    {
        // started profiling, zero out any stale profiling storage
        int depth = i.depth;
        do {
            i.entry[depth].tv_sec = 0;
            i.entry[depth].tv_usec = 0;
            i.reentry[depth].tv_sec = 0;
            i.reentry[depth].tv_usec = 0;
            i.spent[depth].tv_sec = 0;
            i.spent[depth].tv_usec = 0;
        } while (depth-- > 0);
    }
    else
    {
        // clear out profiling data for this thread
        typedef pair<void *,FProfileData *> foo;
        foreach(const foo p,i.profileData)
            delete p.second;
        i.profileData.clear();
    }
}
void FTrace::GetStack(std::list<void *> &stack /* OUT */)
{
    FTraceThreadInfo *info = getThreadInfo();
    if (info == NULL) return;  // avoid infinite loops!
    FTraceThreadInfo &i(*info);
    stack.clear();
    stack.push_front(i.fn);
    for (int a = i.depth - 1; a >= 0; --a)
        stack.push_back(i.stack[a]);
}
struct profileCompare : public binary_function<const FProfileData &, const FProfileData &, bool> {
    bool operator()(const FProfileData &a, const FProfileData &b)
        {
            if (a.mSpent < b.mSpent)
                return false;
            else
                return true;
        }
};

void FTrace::DumpProfiling(unsigned int num)
{
    // log the num worst offenders
    FTraceThreadInfo *info = getThreadInfo();
    if (info == NULL) return;  // avoid infinite loops!
    FTraceThreadInfo &i(*info);
    if (!i.profiling)
    {
        hlog(HLOG_ERR, "profiling not enabled for thread");
        return;
    }
    typedef pair<void *,FProfileData *> foo;
    std::set<FProfileData, profileCompare> profileSet;
    foreach(const foo &pdp,i.profileData)
    {
        const FProfileData * const &pd(pdp.second);
        profileSet.insert(*pd);
    }
    hlog(HLOG_DEBUG, "profiling information collected on %llu distinct functions",
         (u64) i.profileData.size());
    if (num == 0)
        num = profileSet.size();
    unsigned int c = 0;
    foreach (const FProfileData &pd, profileSet)
    {
        if (c++ >= num) break;
        hlog(HLOG_DEBUG, "%016llx: %lld calls, "
             "%04lld.%06lld spent, %04lld.%06lld total", 
             (u64) pd.mFunction, (u64) pd.mCalls, (u64) pd.mSpent.tv_sec, 
             (u64) pd.mSpent.tv_usec, (u64) pd.mTotalSpent.tv_sec, 
             (u64) pd.mTotalSpent.tv_usec);
        if (pd.mStackSample.size() > 0)
        {
            FString stack;
            hlog(HLOG_DEBUG, "   stack sample>>> %s", formatStack(pd.mStackSample, stack).c_str());
        }
    }
}
FString & FTrace::formatStack(const std::list<void *> &stack, FString &out)
{
    out.reserve(18*stack.size());
    out.clear();
    bool first = true;
    std::list<void *>::const_iterator i;
    for(i = stack.begin(); i != stack.end(); ++i)
    {
        out.append(FString(FStringFC(), "%s%016llx", (first)?"":", ",(unsigned long long)*i));
        first = false;
    }
    return out;
}
