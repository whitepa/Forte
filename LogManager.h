#ifndef LogManager_h
#define LogManager_h

#include <vector>
#include <map>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "AutoMutex.h"
#include "Exception.h"
#include "Thread.h"
#include "ThreadKey.h"
#include <syslog.h>
#include <stdarg.h>

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
#define HLOG_ALL         0x00FFF000
#define HLOG_NODEBUG     0x00FE0000

#define HLOG_MIN         HLOG_DEBUG4
#define HLOG_MAX         HLOG_EMERG
#define HLOG_WARNING     HLOG_WARN
#define HLOG_ERROR       HLOG_ERR
#define HLOG_DEBUG0      HLOG_DEBUG

EXCEPTION_SUBCLASS(CForteException, CForteLogException);

class CLogMsg {
public:
    CLogMsg();
    struct timeval mTime;
    int mLevel;
    FString mHost;
    CThread *mThread;
    FString mFunction;
    FString mFile;
    int mLine;
    struct in_addr mClient;
    FString mPrefix;
    FString mMsg;
};

class CLogFilter {
public:
    CLogFilter(const FString sourceFile, int mask) :
        mSourceFile(sourceFile), mMask(mask), mMode(0) {};
    FString mSourceFile;
    int mMask;
    int mMode; ///< FUTURE USE:  include (only) / exclude
};

class CLogfile {
public:
    CLogfile(const FString &path, std::ostream *stream,
             unsigned logMask = HLOG_ALL, bool delStream = false);
    virtual ~CLogfile();
    FString mPath;
    std::ostream *mOut;
    bool mDelStream;
    unsigned mLogMask; // bitmask of desired log levels
    // source file specific log masks
    std::map<FString,unsigned int> mFileMasks;

    void FilterSet(const CLogFilter &filter);
    void FilterDelete(const char *filename);
    void FilterDeleteAll(void);
    void FilterList(std::vector<CLogFilter> &filters);
    
    virtual void Write(const CLogMsg& msg);
    virtual FString FormatMsg(const CLogMsg &msg);
    virtual bool Reopen() { return false; }  // true is reopened, false is not
    static FString getLevelStr(int level);
};

class CSysLogfile : public CLogfile {
public:
    CSysLogfile(const FString& ident, unsigned logMask = HLOG_ALL,
                int option = LOG_NDELAY | LOG_NOWAIT);
    virtual ~CSysLogfile();
    virtual void Write(const CLogMsg& msg);
    virtual FString FormatMsg(const CLogMsg &msg);
    virtual bool Reopen() { return true; }
    FString getIdent() const { return mIdent; }
protected:
    const FString mIdent;
};

class CLogManager;
class CLogContext {
public:
    CLogContext(); // push onto the stack
    virtual ~CLogContext();        // pop from the stack
    
    inline void setClient(struct in_addr client) { mClient = client; }
    inline void setPrefix(const FString &prefix) { mPrefix = prefix; }
    void appendPrefix(const FString &prefix) { mPrefix.append(prefix); }
    struct in_addr mClient;
    FString mPrefix;
protected:
    CLogManager *mLogMgrPtr;
};

class CLogContextStack {
public:
    CLogContextStack() {};
    ~CLogContextStack() {};
    
    std::vector<CLogContext*> mStack;
};

class CLogThreadInfo {
    friend class CLogManager;
public:
    CLogThreadInfo(CLogManager &mgr, CThread &thr);
    virtual ~CLogThreadInfo();
    
protected:
    CLogManager &mLogMgr;
    CThread &mThread;
};

class CLogManager {
    friend class CLogContext;
    friend class CLogThreadInfo;
public:
    CLogManager();
    virtual ~CLogManager();
    
    inline static CLogManager* GetInstancePtr() { return sLogManager; }
    inline static CLogManager & getInstance() { 
        if (sLogManager) return *sLogManager; else throw CForteEmptyReferenceException("no log manager instance");
    }

    void BeginLogging(); // start logging on stderr
    void BeginLogging(const char *path);  // log to a file, can be '//stdout' or '//stderr'
    void BeginLogging(const char *path, int mask);  // log to a file, can be '//stdout' or '//stderr'
    void BeginLogging(CLogfile *logfile); // log to a custom CLogfile object; takes ownership
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

    void PathFilterList(const char *path, std::vector<CLogFilter> &filters);
    void PathList(std::vector<FString> &paths);
    
    void LogMsg(int level, const char *fmt, ...);
    void LogMsgVa(int level, const char *fmt, va_list ap);
    void LogMsgVa(const char * func, const char *file, int line, int level, const char *fmt, va_list ap);

    static FString LogMaskStr(int mask);

protected:
    friend class CMutex;
    std::vector<CLogfile*> mLogfiles;
    CThreadKey mLogContextStackKey;
    CThreadKey mThreadInfoKey;
    CMutex mLogMutex;
    unsigned mLogMaskTemplate;
    static CLogManager *sLogManager;
    
    // Source File specific log masks (global)
    std::map<FString,unsigned int> mFileMasks;

    void beginLogging(const char *path, int mask);  // helper - no locking
    void endLogging(const char *path);    // helper - no locking

    CLogfile &getLogfile(const char *path); // get logfile object for a given path
};

// hermes API compatibility:
//void hlog(int level, const char* fmt, ...);

void _hlog(const char *func, const char *file, int line, int level, const char * fmt, ...);
#define hlog(level, fmt...) _hlog(__FUNCTION__, __FILE__, __LINE__, level, fmt)

#endif
