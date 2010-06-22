#ifndef LogManager_h
#define LogManager_h

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "AutoMutex.h"
#include "Exception.h"
#include "Object.h"
#include "Thread.h"
#include "ThreadKey.h"
#include <syslog.h>
#include <stdarg.h>

#define HLOG_TRACE       0x00000800
#define HLOG_DEBUG4      0x00001000
#define HLOG_DEBUG3      0x00002000
#define HLOG_DEBUG2      0x00004000
#define HLOG_DEBUG1      0x00008000
#define HLOG_DEBUG       0x00010000
#define HLOG_INFO        0x00020000
#define HLOG_NOTICE      0x00040000
#define HLOG_WARN        0x00080000
#define HLOG_ERR         0x00100000
#define HLOG_CRIT        0x00200000
#define HLOG_ALERT       0x00400000
#define HLOG_EMERG       0x00800000
#define HLOG_ALL         0x00FFFFFF
#define HLOG_NODEBUG     0x00FE0000

#define HLOG_MIN         HLOG_TRACE
#define HLOG_MAX         HLOG_EMERG
#define HLOG_WARNING     HLOG_WARN
#define HLOG_ERROR       HLOG_ERR
#define HLOG_DEBUG0      HLOG_DEBUG

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, ELog);

    class LogMsg : public Object {
    public:
        LogMsg();
        struct timeval mTime;
        int mLevel;
        FString mHost;
        Thread *mThread;
        FString mFunction;
        FString mFile;
        int mLine;
        struct in_addr mClient;
        FString mPrefix;
        FString mMsg;
    };

    class LogFilter : public Object {
    public:
        LogFilter(const FString sourceFile, int mask) :
            mSourceFile(sourceFile), mMask(mask), mMode(0) {};
        FString mSourceFile;
        int mMask;
        int mMode; ///< FUTURE USE:  include (only) / exclude
    };

    class Logfile : public Object {
    public:
        Logfile(const FString &path, std::ostream *stream,
                 unsigned logMask = HLOG_ALL, bool delStream = false);
        virtual ~Logfile();
        FString mPath;
        std::ostream *mOut;
        bool mDelStream;
        unsigned mLogMask; // bitmask of desired log levels
        // source file specific log masks
        std::map<FString,unsigned int> mFileMasks;

        void FilterSet(const LogFilter &filter);
        void FilterDelete(const char *filename);
        void FilterDeleteAll(void);
        void FilterList(std::vector<LogFilter> &filters);
    
        virtual void Write(const LogMsg& msg);
        virtual FString FormatMsg(const LogMsg &msg);
        virtual bool Reopen() { return false; }  // true is reopened, false is not
        static FString GetLevelStr(int level);
    };

    class SysLogfile : public Logfile {
    public:
        SysLogfile(const FString& ident, unsigned logMask = HLOG_ALL,
                    int option = LOG_NDELAY | LOG_NOWAIT);
        virtual ~SysLogfile();
        virtual void Write(const LogMsg& msg);
        virtual FString FormatMsg(const LogMsg &msg);
        virtual bool Reopen() { return true; }
        FString GetIdent() const { return mIdent; }
     protected:
        const FString mIdent;
    };

    class LogManager;
    class LogContext : public Object {
    public:
        LogContext(); // push onto the stack
        virtual ~LogContext();        // pop from the stack
    
        inline void SetClient(struct in_addr client) { mClient = client; }
        inline void SetPrefix(const FString &prefix) { mPrefix = prefix; }
        void AppendPrefix(const FString &prefix) { mPrefix.append(prefix); }
        struct in_addr mClient;
        FString mPrefix;
     protected:
        LogManager *mLogMgrPtr;
    };

    class LogContextStack : public Object {
    public:
        LogContextStack() {};
        ~LogContextStack() {};
    
        std::vector<LogContext*> mStack;
    };

    class LogThreadInfo : public Object {
        friend class LogManager;
    public:
        LogThreadInfo(LogManager &mgr, Thread &thr);
        virtual ~LogThreadInfo();
    
    protected:
        LogManager &mLogMgr;
        Thread &mThread;
    };

    class LogManager : public Object {
        friend class LogContext;
        friend class LogThreadInfo;
    public:
        LogManager();
        virtual ~LogManager();
    
        inline static LogManager* GetInstancePtr() { return sLogManager; }
        inline static LogManager & GetInstance() { 
            if (sLogManager) return *sLogManager; else throw EEmptyReference("no log manager instance");
        }

        void BeginLogging(); // start logging on stderr
        void BeginLogging(const char *path);  // log to a file, can be '//stdout' or '//stderr'
        void BeginLogging(const char *path, int mask);  // log to a file, can be '//stdout' or '//stderr'
        void BeginLogging(Logfile *logfile); // log to a custom Logfile object; takes ownership
        void EndLogging();   // stop all logging
        void EndLogging(const char *path); // stop logging to a specific location
        void Reopen();  // re-open all log files

        void SetLogMask(const char *path, unsigned int mask); // bitmask of desired log levels
        unsigned int GetLogMask(const char *path);
        void SetGlobalLogMask(unsigned int mask); // set log mask on ALL log files

        void SetSourceFileLogMask(const char *filename, unsigned int mask);
        void ClearSourceFileLogMask(const char *filename);

        void PathSetSourceFileLogMask(const char *path, const char *filename, unsigned int mask);
        void PathClearSourceFileLogMask(const char *path, const char *filename);
        void PathClearAllSourceFiles(const char *path);

        void PathFilterList(const char *path, std::vector<LogFilter> &filters);
        void PathList(std::vector<FString> &paths);
    
        void Log(int level, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
        void LogMsgVa(int level, const char *fmt, va_list ap);
        void LogMsgVa(const char * func, const char *file, int line, int level, const char *fmt, va_list ap);

        static FString LogMaskStr(int mask);

    protected:
        friend class Mutex;
        std::vector<Logfile*> mLogfiles;
        ThreadKey mLogContextStackKey;
        ThreadKey mThreadInfoKey;
        Mutex mLogMutex;
        unsigned mLogMaskTemplate;
        static LogManager *sLogManager;
    
        // Source File specific log masks (global)
        std::map<FString,unsigned int> mFileMasks;

        void beginLogging(const char *path, int mask);  // helper - no locking
        void endLogging(const char *path);    // helper - no locking

        Logfile &getLogfile(const char *path); // get logfile object for a given path
    };
};
// hermes API compatibility:
//void hlog(int level, const char* fmt, ...);

void _hlog(const char *func, const char *file, int line, int level, const char * fmt, ...) __attribute__((format(printf, 5, 6)));
#define hlog(level, fmt...) _hlog(__FUNCTION__, __FILE__, __LINE__, level, fmt)

#endif
