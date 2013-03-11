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
        enum
        {
            MAX_BUFFER_SIZE = 512
        };

        FunctionEntry(const char *functionName, const char *file, int line)
            : mFN(functionName), mFile(file), mLine(line), mArgumentBuffer(NULL)
        {
            _hlog(mFN.c_str(), mFile, mLine, HLOG_TRACE, "ENTER");
        }

        FunctionEntry(const char *functionName, const char *file, int line, const char*fmt, ...) __attribute__((format(printf, 5, 6)))
            : mFN(functionName), mFile(file), mLine(line), mArgumentBuffer(NULL)
        {
            va_list args;
            va_start(args, fmt);
            mArgumentBuffer = static_cast<char *>(malloc(MAX_BUFFER_SIZE));
            vsnprintf(mArgumentBuffer, MAX_BUFFER_SIZE, fmt, args);

            _hlog(mFN.c_str(), mFile, mLine,
                  HLOG_TRACE, "ENTER (%s)", mArgumentBuffer);
            va_end(args);
        }

        FunctionEntry(const char* functionName, const char *file, int line, const Forte::FString& message)
            : mFN(functionName), mFile(file), mLine(line), mArgumentBuffer(NULL)
        {
            mArgumentBuffer = static_cast<char *>(malloc(MAX_BUFFER_SIZE));
            strncpy(mArgumentBuffer, message.c_str(), MAX_BUFFER_SIZE);
            _hlog(mFN.c_str(), mFile, mLine, HLOG_TRACE, "ENTER (%s)", mArgumentBuffer);
        }

        ~FunctionEntry() {
            if (mArgumentBuffer)
            {
                _hlog(mFN.c_str(), mFile, mLine, HLOG_TRACE, "EXIT (%s)",
                      mArgumentBuffer);
                free(mArgumentBuffer);
            }
            else
            {
                _hlog(mFN.c_str(), mFile, mLine, HLOG_TRACE, "EXIT");
            }
        }
    protected:
        FString mFN;
        FString mFile;
        int mLine;
        char *mArgumentBuffer;
    };

    class FunctionEntryProxy
    {
    public:
        FunctionEntryProxy(const char *functionName, const char *file, int line)
        {
            if(_should_hlog(functionName, file, line, HLOG_TRACE))
            {
                mImpl.reset(new FunctionEntry(functionName, file, line));
            }
        }

        FunctionEntryProxy(const char *functionName, const char *file, int line, const char*fmt, ...) __attribute__((format(printf, 5, 6)))
        {
            if(_should_hlog(functionName, file, line, HLOG_TRACE))
            {
                va_list args;
                va_start(args, fmt);
                char message[FunctionEntry::MAX_BUFFER_SIZE];
                vsnprintf(message, FunctionEntry::MAX_BUFFER_SIZE, fmt, args);

                const FString strMessage(message, sizeof(message));
                mImpl.reset(new FunctionEntry(functionName, file, line, strMessage));

                va_end(args);
            }
        }

        FunctionEntryProxy(const char* functionName, const char *file, int line, const Forte::FString& message)
        {
            if(_should_hlog(functionName, file, line, HLOG_TRACE))
            {
                mImpl.reset(new FunctionEntry(functionName, file, line, message));
            }
        }

    private:
        boost::scoped_ptr<FunctionEntry> mImpl;
    };
};

//#ifdef FORTE_FUNCTION_TRACING
/**
 * Prints entrance and exit log lines at the begin/end of the
 * scope.
 * This macro passes function name, file, and line.
 */
#define FTRACE \
Forte::FunctionEntryProxy _forte_trace_object(__PRETTY_FUNCTION__, __FILE__, __LINE__)


//@ todo: optimize stream out of hot path
#define FTRACESTREAM(message) { \
    std::ostringstream o; \
    o <<  message; \
    Forte::FunctionEntryProxy _forte_trace_object(__PRETTY_FUNCTION__, __FILE__, __LINE__, o.str()); } \

/**
 * Same as FTRACE except allows a format string to be passed in
 * as well.
 * Example:
 *    FTRACE2("%s, %i", arg1, arg2)
 *
 *   Output (on enter):
 *     ENTER function(arg1, arg2)
 */
#define FTRACE2(FMT...) Forte::FunctionEntryProxy _forte_trace_object(__PRETTY_FUNCTION__, __FILE__, __LINE__, FMT)
//#else
//#define FTRACE
//#endif

#endif
