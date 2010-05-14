#ifndef __forte_Trace_h__
#define __forte_Trace_h__

#include "ThreadKey.h"
#include "LogManager.h"
#include <list>
#include <map>

// maximum depth for which tracing and profiling data is maintained:
#define FTRACE_MAXDEPTH 256
namespace Forte
{
    class FProfileData {
    public:
        FProfileData(void *fn, const struct timeval &spent, const struct timeval &totalSpent) 
            __attribute__ ((no_instrument_function));
        void AddCall(const struct timeval &spent, const struct timeval &totalSpent) 
            __attribute__ ((no_instrument_function));

        const void *mFunction;
        unsigned int mCalls;
        struct timeval mSpent;
        struct timeval mTotalSpent;
        std::list<void*> mStackSample;
    };

    class FTraceThreadInfo {
    public:
        __attribute__ ((no_instrument_function)) FTraceThreadInfo();
        __attribute__ ((no_instrument_function)) ~FTraceThreadInfo();
        unsigned short depth;
        bool profiling;
        void *stack[FTRACE_MAXDEPTH];
        void *lastfn[FTRACE_MAXDEPTH];
        void *fn;

        // profile data (if enabled):
        struct timeval entry[FTRACE_MAXDEPTH];
        struct timeval reentry[FTRACE_MAXDEPTH];
        struct timeval spent[FTRACE_MAXDEPTH];
    
        void StoreProfile(void *fn, struct timeval &spent, struct timeval &totalSpent) 
            __attribute__ ((no_instrument_function));
        std::map<void*,FProfileData *> profileData;
    };

    class FTrace {
    public:
        static void Enable() __attribute__ ((no_instrument_function));
        static void Disable() __attribute__ ((no_instrument_function));
        static void SetProfiling(bool profile) __attribute__ ((no_instrument_function));
        static void DumpProfiling(unsigned int num = 0) __attribute__ ((no_instrument_function));
        static void Enter(void *fn, void *caller) __attribute__ ((no_instrument_function));
        static void Exit(void *fn, void *caller) __attribute__ ((no_instrument_function));
        static void Cleanup(void *ptr) __attribute__ ((no_instrument_function));

        static unsigned int GetDepth(void) __attribute__ ((no_instrument_function));
        static void GetStack(std::list<void *> &stack /* OUT */) __attribute__ ((no_instrument_function));
        static FString & formatStack(const std::list<void *> &stack, FString &out) __attribute__ ((no_instrument_function));

        static unsigned int sInitialized;
     protected:
        static FTraceThreadInfo* getThreadInfo(void) __attribute__ ((no_instrument_function));
        static ThreadKey sTraceKey;
        static FTrace *sInstance;
    };

    class FunctionEntry
    {
    public:
        FunctionEntry(const char *functionName) : mFN(functionName) {
            hlog(HLOG_DEBUG, "ENTER %s", mFN.c_str());
        }
        virtual ~FunctionEntry() {
            hlog(HLOG_DEBUG, "EXIT %s", mFN.c_str());
        }
    protected:
        FString mFN;
    };
};

//#ifdef FORTE_FUNCTION_TRACING
#define FTRACE Forte::FunctionEntry _forte_trace_object(__FUNCTION__)
//#else
//#define FTRACE
//#endif
    
#endif
