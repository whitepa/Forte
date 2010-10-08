#include <errno.h>
#include <string.h>

#include "Foreach.h"
#include "FTrace.h"
#include "LogManager.h"
#include <stdarg.h>
#include <sys/time.h>

using namespace Forte;
using namespace std;

LogManager *LogManager::sLogManager = NULL;

// Logfile
Logfile::Logfile(const FString& path, ostream *str, unsigned logMask, bool delStream) :
    mPath(path),
    mOut(str),
    mDelStream(delStream),
    mLogMask(logMask)
{
//    mOut.tie(&str);
}

Logfile::~Logfile()
{
    if (mDelStream)
        delete mOut;
}

void Logfile::Write(const LogMsg& msg)
{
    if (mOut)
    {
        (*mOut) << FormatMsg(msg);
        mOut->flush();
    }
}

FString Logfile::GetLevelStr(int level)
{
    FString levelstr;

    switch (level & HLOG_ALL)
    {
    case HLOG_TRACE:
        levelstr = "TRCE";
        break;
    case HLOG_DEBUG4:
    case HLOG_DEBUG3:
    case HLOG_DEBUG2:
    case HLOG_DEBUG1:
    case HLOG_DEBUG:
        levelstr = "DBUG";
        break;
    case HLOG_INFO:
        levelstr = "INFO";
        break;
    case HLOG_NOTICE:
        levelstr = "NOTC";
        break;
    case HLOG_WARN:
        levelstr = "WARN";
        break;
    case HLOG_ERR:
        levelstr = "ERR ";
        break;
    case HLOG_CRIT:
        levelstr = "CRIT";
        break;
    case HLOG_ALERT:
        levelstr = "ALRT";
        break;
    case HLOG_EMERG:
        levelstr = "EMRG";
        break;
    default:
        levelstr = "????";
        break;
    }

    return levelstr;
}

void Logfile::FilterSet(const LogFilter &filter)
{
    mFileMasks[filter.mSourceFile] = filter.mMask;
}
void Logfile::FilterDelete(const char *filename)
{
    mFileMasks.erase(filename);
}
void Logfile::FilterDeleteAll(void)
{
    mFileMasks.clear();
}
void Logfile::FilterList(std::vector<LogFilter> &filters)
{
    filters.clear();
    typedef std::pair<FString, unsigned int> FileMaskPair;
    foreach(const FileMaskPair &fp, mFileMasks)
        filters.push_back(LogFilter(fp.first, fp.second));
}

FString Logfile::FormatMsg(const LogMsg &msg)
{
    FString levelstr, formattedMsg;
    struct tm lt;
    levelstr = GetLevelStr(msg.mLevel);
    localtime_r(&(msg.mTime.tv_sec), &lt);

    FString thread(FStringFC(), "[%s]",(msg.mThread) ? msg.mThread->mThreadName.c_str() : "unknown");
    int padT = 25-thread.size();
    if (padT<0) padT=0;
    thread.append(padT,' ');

    FString fileLine(FStringFC(), "%s:%d", msg.mFile.c_str(), msg.mLine);
    int padFL = 35-fileLine.size();
    if (padFL<0) padFL=0;
    fileLine.append(padFL,' ');

    unsigned int depth = FTrace::GetDepth();
    FString offset;
    offset.append(depth * 3, ' ');
    formattedMsg.Format("%02d/%02d/%02d %02d:%02d:%02d.%03d %s%s%s|%s%s() %s%s\n",
                        lt.tm_mon + 1, lt.tm_mday, lt.tm_year % 100,
                        lt.tm_hour, lt.tm_min, lt.tm_sec, 
                        (int)(msg.mTime.tv_usec/1000),
                        levelstr.c_str(),
                        thread.c_str(),
                        fileLine.c_str(), offset.c_str(),
                        msg.mFunction.c_str(), msg.mPrefix.c_str(),
                        msg.mMsg.c_str());
    return formattedMsg;
}


// SysLogfile
SysLogfile::SysLogfile(const FString& ident, unsigned logMask, int option)
:
    Logfile("//syslog/" + ident, NULL, logMask, false),
    mIdent(ident)
{
    // use mIdent.c_str() so that the memory associated with ident stays valid
    // while the syslog connection is open
    openlog(mIdent.c_str(), option, LOG_DAEMON);
}

SysLogfile::~SysLogfile()
{
    closelog();
}

void SysLogfile::Write(const LogMsg& msg)
{
    int level = LOG_DEBUG;

    // get highest syslog level
    if (msg.mLevel & HLOG_INFO) level = LOG_INFO;
    if (msg.mLevel & HLOG_NOTICE) level = LOG_NOTICE;
    if (msg.mLevel & HLOG_WARN) level = LOG_WARNING;
    if (msg.mLevel & HLOG_ERR) level = LOG_ERR;
    if (msg.mLevel & HLOG_CRIT) level = LOG_CRIT;
    if (msg.mLevel & HLOG_ALERT) level = LOG_ALERT;
    if (msg.mLevel & HLOG_EMERG) level = LOG_EMERG;

    // log to syslog
    syslog(LOG_DAEMON | level, "%s", FormatMsg(msg).c_str());
}

FString SysLogfile::FormatMsg(const LogMsg &msg)
{
    FString levelstr, formattedMsg;
    levelstr = GetLevelStr(msg.mLevel);
    formattedMsg.Format("%s [%s] %s\n",
                        levelstr.c_str(),
                        (msg.mThread) ? msg.mThread->mThreadName.c_str() : "unknown",
                        msg.mMsg.c_str());
    return formattedMsg;
}


// LogMsg
LogMsg::LogMsg() :
    mThread(NULL)
{}


// LogContext
LogContext::LogContext()
{
    LogManager &mgr(LogManager::GetInstance());
    if (mgr.mLogContextStackKey.Get() == NULL)
        mgr.mLogContextStackKey.Set(new LogContextStack());
    mLogMgrPtr = &mgr;
    LogContextStack *stack = (LogContextStack *)(mgr.mLogContextStackKey.Get());
    if (!(stack->mStack.empty()))
    {
        LogContext *previousContext(stack->mStack.back());
        mClient = previousContext->mClient;
        mPrefix = previousContext->mPrefix;
    }
    stack->mStack.push_back(this);
}

LogContext::~LogContext()
{
    // remove pointer to this log context from the manager
    AutoUnlockMutex lock(mLogMgrPtr->mLogMutex);
    LogContextStack *stack = (LogContextStack *)(mLogMgrPtr->mLogContextStackKey.Get());
    if (stack) stack->mStack.pop_back();
}


// LogThreadInfo
LogThreadInfo::LogThreadInfo(LogManager &mgr, Thread &thr) :
    mLogMgr(mgr),
    mThread(thr)
{
    mLogMgr.mThreadInfoKey.Set(this);
}

LogThreadInfo::~LogThreadInfo()
{
    // remove pointer to this log context from the manager
    AutoUnlockMutex lock(mLogMgr.mLogMutex);
    mLogMgr.mThreadInfoKey.Set(NULL);
}


// LogManager
LogManager::LogManager() {
    mLogMaskTemplate = HLOG_ALL;
}

LogManager::~LogManager() {
    Log(HLOG_INFO, "logging halted");
    EndLogging();
}

void LogManager::InitGlobal(void)
{
    if (!sLogManager) {
        sLogManager = this;
    }
}

void LogManager::BeginLogging()
{
    sLogManager = this;
    BeginLogging("//stderr");
}

void LogManager::BeginLogging(const char *path)
{
    sLogManager = this;
    AutoUnlockMutex lock(mLogMutex);
    beginLogging(path, mLogMaskTemplate);
}

void LogManager::BeginLogging(const char *path, int mask)
{
    sLogManager = this;
    AutoUnlockMutex lock(mLogMutex);
    beginLogging(path, mask);
}

void LogManager::beginLogging(const char *path, int mask)  // helper - no locking
{
    Logfile *logfile;
    if (!strcmp(path, "//stderr"))
        logfile = new Logfile(path, &cerr, mask);
    else if (!strcmp(path, "//stdout"))
        logfile = new Logfile(path, &cout, mask);
    else if (!strncmp(path, "//syslog/", 9))
    {        
        logfile = new SysLogfile(path + 9, mask);
    }
    else
    {
        ofstream *out = new ofstream(path, ios::app | ios::out);
        logfile = new Logfile(path, out, mask, true);
    }
    logfile->mPath = path;
    mLogfiles.push_back(logfile);
}

void LogManager::BeginLogging(Logfile *logfile)
{
    sLogManager = this;
    AutoUnlockMutex lock(mLogMutex);
    mLogfiles.push_back(logfile);
}

void LogManager::EndLogging()
{
    sLogManager = NULL;
    AutoUnlockMutex lock(mLogMutex);
    std::vector<Logfile*>::iterator i;
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
        delete *i;
    mLogfiles.clear();
}

void LogManager::EndLogging(const char *path)
{
    AutoUnlockMutex lock(mLogMutex);
    endLogging(path);
}

void LogManager::endLogging(const char *path)  // helper - no locking
{
    // find the logfile with this path
    std::vector<Logfile*>::iterator i;
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
    {
        if (!(*i)->mPath.Compare(path))
        {
            delete *i;
            mLogfiles.erase(i);
            break;
        }
    }
    if (mLogfiles.empty()) sLogManager = NULL;
}

void LogManager::Reopen()  // re-open all log files
{
    AutoUnlockMutex lock(mLogMutex);
    std::vector<Logfile*>::iterator i;
    typedef std::pair<FString,int> PathMaskPair;
    std::list<PathMaskPair> recreate;

    // call Reopen()
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
    {
        if (!(*i)->Reopen()) recreate.push_back(PathMaskPair((*i)->mPath, (*i)->mLogMask));
    }

    // for anything that could not reopen, re-create it
    foreach(const PathMaskPair &pm, recreate)
    {
        endLogging(pm.first);
        beginLogging(pm.first, pm.second);
    }
}

void LogManager::SetGlobalLogMask(unsigned int mask)
{
    AutoUnlockMutex lock(mLogMutex);
    std::vector<Logfile*>::iterator i;
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
    {
        (*i)->mLogMask = mask;
    }
    mLogMaskTemplate = mask;  // template is for newly created log files
}

void LogManager::SetLogMask(const char *path, unsigned int mask)
{
    AutoUnlockMutex lock(mLogMutex);

    // find the logfile with this path
    std::vector<Logfile*>::iterator i;
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
    {
        if (!(*i)->mPath.Compare(path))
        {
            (*i)->mLogMask = mask;
        }
    }
}
unsigned int LogManager::GetLogMask(const char *path)
{
    AutoUnlockMutex lock(mLogMutex);

    // find the logfile with this path
    std::vector<Logfile*>::iterator i;
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
        if (!(*i)->mPath.Compare(path))
            return (*i)->mLogMask;
    return 0;
}
void LogManager::SetSourceFileLogMask(const char *filename, unsigned int mask)
{
    AutoUnlockMutex lock(mLogMutex);
    mFileMasks[filename] = mask;
}
void LogManager::ClearSourceFileLogMask(const char *filename)
{
    AutoUnlockMutex lock(mLogMutex);
    mFileMasks.erase(filename);
}


void LogManager::PathSetSourceFileLogMask(const char *path, const char *filename, 
                                           unsigned int mask)
{
    AutoUnlockMutex lock(mLogMutex);
    Logfile &logfile(getLogfile(path));
    logfile.FilterSet(LogFilter(filename, mask));
}
void LogManager::PathClearSourceFileLogMask(const char *path, const char *filename)
{
    AutoUnlockMutex lock(mLogMutex);
    Logfile &logfile(getLogfile(path));
    logfile.FilterDelete(filename);
}
void LogManager::PathClearAllSourceFiles(const char *path)
{
    AutoUnlockMutex lock(mLogMutex);
    Logfile &logfile(getLogfile(path));
    logfile.FilterDeleteAll();
}

void LogManager::PathFilterList(const char *path, std::vector<LogFilter> &filters)
{
    AutoUnlockMutex lock(mLogMutex);
    Logfile &logfile(getLogfile(path));
    logfile.FilterList(filters);    
}

void LogManager::PathList(std::vector<FString> &paths)
{
    AutoUnlockMutex lock(mLogMutex);
    foreach (const Logfile *logfile, mLogfiles)
        paths.push_back(logfile->mPath);
}

Logfile & LogManager::getLogfile(const char *path)
{
    // internal helper to get a logfile object (caller should already be locked)
    foreach (Logfile *logfile, mLogfiles)
        if (!(logfile->mPath.Compare(path)))
            return *logfile;
    throw ELog(FStringFC(), "not currently logging to '%s'", path);
}

void LogManager::Log(int level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    LogMsgVa(level, fmt, ap);
    va_end(ap);
}

void LogManager::LogMsgVa(int level, const char *fmt, va_list ap)
{
    LogMsgVa(NULL, NULL, 0, level, fmt, ap);
}

void LogManager::LogMsgVa(const char * func, const char * file, int line, int level, const char *fmt, va_list ap)
{
    char tmp[128];
    tmp[0] = 0;
    LogMsg msg;
    if (level < HLOG_MIN)
    {
        // convert syslog level to HLOG level
        switch (level)
        {
        case LOG_DEBUG: level = HLOG_DEBUG; break;
        case LOG_INFO: level = HLOG_INFO; break;
        case LOG_NOTICE: level = HLOG_NOTICE; break;
        case LOG_WARNING: level = HLOG_WARN; break;
        case LOG_ERR: level = HLOG_ERR; break;
        case LOG_CRIT: level = HLOG_CRIT; break;
        case LOG_ALERT: level = HLOG_ALERT; break;
        case LOG_EMERG: level = HLOG_EMERG; break;
        default: level = HLOG_CRIT; break;
        } 
    }
    gettimeofday(&(msg.mTime), NULL);
    if (gethostname(tmp, sizeof(tmp))==0)
        msg.mHost.assign(tmp);
    else
        msg.mHost = "(hostname too long)";
    AutoUnlockMutex lock(mLogMutex);
    LogThreadInfo *ti = (LogThreadInfo *)mThreadInfoKey.Get();
    if (ti != NULL)
        msg.mThread = &(ti->mThread);
    msg.mLevel = level;
    char *amsg;
    vasprintf(&amsg, fmt, ap);
    msg.mMsg = amsg;
    msg.mFunction = func;
    msg.mFile = file;
    msg.mLine = line;
    LogContextStack *stack = (LogContextStack*)mLogContextStackKey.Get();
    if (stack && !(stack->mStack.empty()))
    {
        LogContext *c(stack->mStack.back());
        msg.mClient = c->mClient;
        msg.mPrefix = c->mPrefix;
    }
    free(amsg);

    // decide whether this message should be logged anywhere.
    // Decision is made by (in order of precedence):
    //   Output Path: Source file specific mask
    //   Global:      Source file specific mask
    //   Output Path: Global Mask

    bool fileSpecific = false;
    // check for file specific log mask
    if (mFileMasks.find(file) != mFileMasks.end())
    {
        if ((level & mFileMasks[file]) != 0)
            fileSpecific = true;
        else
            return; // do not log anywhere
    }        

    // send the message to all the logfiles
    foreach (Logfile *i, mLogfiles)
    {
        // first check this logfile's source file filters
        bool pathFileSpecific = false;
        Logfile &lf(*i);
        if (lf.mFileMasks.find(file) != lf.mFileMasks.end())
        {
            if ((level & lf.mFileMasks[file]) != 0)
                pathFileSpecific = true;
            else
                continue; // don't log to this path
        }
        if (fileSpecific || 
            pathFileSpecific ||
            (level & (lf.mLogMask)) != 0)
        {
            lf.Write(msg);
        }
    }
}

// hermes API compatibility:
void _hlog(const char *func, const char * file, int line, int level, const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    try
    {
        LogManager *log_mgr = LogManager::GetInstancePtr();
        if (log_mgr != NULL) log_mgr->LogMsgVa(func, file, line, level, fmt, ap);
    }
    catch (...)
    {
    }
    va_end(ap);
}

void _hlog_errno(const char* func, const char* file, int line, int level)
{
    char  err_buf[HLOG_ERRNO_BUF_LEN];

    memset(err_buf, 0, HLOG_ERRNO_BUF_LEN);
    strerror_r(errno, err_buf, HLOG_ERRNO_BUF_LEN-1);

    _hlog(func, file, line, level, "strerror: %s", err_buf);
}

static const char *levelstr[] = 
{
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10",
    "TRACE",
    "DEBUG4",
    "DEBUG3",
    "DEBUG2",
    "DEBUG1",
    "DEBUG",
    "INFO",
    "NOTICE",
    "WARN",
    "ERR",
    "CRIT",
    "ALERT",
    "EMERG",
    "24",
    "25",
    "26",
    "27",
    "28",
    "29",
    "30",
    "31",
    NULL
};

    

// helper to generate a string describing a log mask
FString LogManager::LogMaskStr(int mask)
{
    std::vector<FString> levels;
    for (int i = 0; i < 32; ++i)
    {
        int bit = 1 << i;
        if (mask & bit)
        {
            levels.push_back(levelstr[i]);
        }
    }
    return FString::Join(levels, "|");
}
