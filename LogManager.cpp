#include "Forte.h"
#include <stdarg.h>
#include <sys/time.h>

// XXX auto_ptr was deleting itself too early, segfault on shutdown

CLogManager *CLogManager::sLogManager = NULL;

// CLogfile
CLogfile::CLogfile(const FString& path, ostream *str, unsigned logMask, bool delStream) :
    mPath(path),
    mOut(str),
    mDelStream(delStream),
    mLogMask(logMask)
{
//    mOut.tie(&str);
}

CLogfile::~CLogfile()
{
    if (mDelStream)
        delete mOut;
}

void CLogfile::Write(const CLogMsg& msg)
{
    if (mOut)
    {
        (*mOut) << FormatMsg(msg);
        mOut->flush();
    }
}

FString CLogfile::getLevelStr(int level)
{
    FString levelstr;

    switch (level & HLOG_ALL)
    {
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

void CLogfile::FilterSet(const CLogFilter &filter)
{
    mFileMasks[filter.mSourceFile] = filter.mMask;
}
void CLogfile::FilterDelete(const char *filename)
{
    mFileMasks.erase(filename);
}
void CLogfile::FilterDeleteAll(void)
{
    mFileMasks.clear();
}
void CLogfile::FilterList(std::vector<CLogFilter> &filters)
{
    filters.clear();
    typedef std::pair<FString, unsigned int> FileMaskPair;
    foreach(const FileMaskPair &fp, mFileMasks)
        filters.push_back(CLogFilter(fp.first, fp.second));
}

FString CLogfile::FormatMsg(const CLogMsg &msg)
{
    FString levelstr, formattedMsg;
    struct tm lt;
    levelstr = getLevelStr(msg.mLevel);
    localtime_r(&(msg.mTime.tv_sec), &lt);

    FString thread(FStringFC(), "[%s]",(msg.mThread) ? msg.mThread->mThreadName.c_str() : "unknown");
    int padT = 25-thread.size();
    if (padT<0) padT=0;
    thread.append(padT,' ');

    FString fileLine(FStringFC(), "%s:%d", msg.mFile.c_str(), msg.mLine);
    int padFL = 35-fileLine.size();
    if (padFL<0) padFL=0;
    fileLine.append(padFL,' ');

    unsigned int depth = FTrace::getDepth();
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


// CSysLogfile
CSysLogfile::CSysLogfile(const FString& ident, unsigned logMask, int option)
:
    CLogfile("//syslog/" + ident, NULL, logMask, false),
    mIdent(ident)
{
    // use mIdent.c_str() so that the memory associated with ident stays valid
    // while the syslog connection is open
    openlog(mIdent.c_str(), option, LOG_DAEMON);
}

CSysLogfile::~CSysLogfile()
{
    closelog();
}

void CSysLogfile::Write(const CLogMsg& msg)
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

FString CSysLogfile::FormatMsg(const CLogMsg &msg)
{
    FString levelstr, formattedMsg;
    levelstr = getLevelStr(msg.mLevel);
    formattedMsg.Format("%s [%s] %s\n",
                        levelstr.c_str(),
                        (msg.mThread) ? msg.mThread->mThreadName.c_str() : "unknown",
                        msg.mMsg.c_str());
    return formattedMsg;
}


// CLogMsg
CLogMsg::CLogMsg() :
    mThread(NULL)
{}


// CLogContext
CLogContext::CLogContext()
{
    CLogManager &mgr(CLogManager::getInstance());
    if (mgr.mLogContextStackKey.get() == NULL)
        mgr.mLogContextStackKey.set(new CLogContextStack());
    mLogMgrPtr = &mgr;
    CLogContextStack *stack = (CLogContextStack *)(mgr.mLogContextStackKey.get());
    if (!(stack->mStack.empty()))
    {
        CLogContext *previousContext(stack->mStack.back());
        mClient = previousContext->mClient;
        mPrefix = previousContext->mPrefix;
    }
    stack->mStack.push_back(this);
}

CLogContext::~CLogContext()
{
    // remove pointer to this log context from the manager
    CAutoUnlockMutex lock(mLogMgrPtr->mLogMutex);
    CLogContextStack *stack = (CLogContextStack *)(mLogMgrPtr->mLogContextStackKey.get());
    if (stack) stack->mStack.pop_back();
}


// CLogThreadInfo
CLogThreadInfo::CLogThreadInfo(CLogManager &mgr, CThread &thr) :
    mLogMgr(mgr),
    mThread(thr)
{
    mLogMgr.mThreadInfoKey.set(this);
}

CLogThreadInfo::~CLogThreadInfo()
{
    // remove pointer to this log context from the manager
    CAutoUnlockMutex lock(mLogMgr.mLogMutex);
    mLogMgr.mThreadInfoKey.set(NULL);
}


// CLogManager
CLogManager::CLogManager() {
    mLogMaskTemplate = HLOG_ALL;
}

CLogManager::~CLogManager() {
    LogMsg(HLOG_INFO, "logging halted");
    EndLogging();
}

void CLogManager::BeginLogging()
{
    sLogManager = this;
    BeginLogging("//stderr");
}

void CLogManager::BeginLogging(const char *path)
{
    sLogManager = this;
    CAutoUnlockMutex lock(mLogMutex);
    beginLogging(path, mLogMaskTemplate);
}

void CLogManager::BeginLogging(const char *path, int mask)
{
    sLogManager = this;
    CAutoUnlockMutex lock(mLogMutex);
    beginLogging(path, mask);
}

void CLogManager::beginLogging(const char *path, int mask)  // helper - no locking
{
    CLogfile *logfile;
    if (!strcmp(path, "//stderr"))
        logfile = new CLogfile(path, &cerr, mask);
    else if (!strcmp(path, "//stdout"))
        logfile = new CLogfile(path, &cout, mask);
    else if (!strncmp(path, "//syslog/", 9))
    {        
        logfile = new CSysLogfile(path + 9, mask);
    }
    else
    {
        ofstream *out = new ofstream(path, ios::app | ios::out);
        logfile = new CLogfile(path, out, mask, true);
    }
    logfile->mPath = path;
    mLogfiles.push_back(logfile);
}

void CLogManager::BeginLogging(CLogfile *logfile)
{
    sLogManager = this;
    CAutoUnlockMutex lock(mLogMutex);
    mLogfiles.push_back(logfile);
}

void CLogManager::EndLogging()
{
    sLogManager = NULL;
    CAutoUnlockMutex lock(mLogMutex);
    std::vector<CLogfile*>::iterator i;
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
        delete *i;
    mLogfiles.clear();
}

void CLogManager::EndLogging(const char *path)
{
    CAutoUnlockMutex lock(mLogMutex);
    endLogging(path);
}

void CLogManager::endLogging(const char *path)  // helper - no locking
{
    // find the logfile with this path
    std::vector<CLogfile*>::iterator i;
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

void CLogManager::Reopen()  // re-open all log files
{
    CAutoUnlockMutex lock(mLogMutex);
    std::vector<CLogfile*>::iterator i;
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

void CLogManager::SetGlobalLogMask(unsigned int mask)
{
    CAutoUnlockMutex lock(mLogMutex);
    std::vector<CLogfile*>::iterator i;
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
    {
        (*i)->mLogMask = mask;
    }
    mLogMaskTemplate = mask;  // template is for newly created log files
}

void CLogManager::SetLogMask(const char *path, unsigned int mask)
{
    CAutoUnlockMutex lock(mLogMutex);

    // find the logfile with this path
    std::vector<CLogfile*>::iterator i;
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
    {
        if (!(*i)->mPath.Compare(path))
        {
            (*i)->mLogMask = mask;
        }
    }
}
unsigned int CLogManager::GetLogMask(const char *path)
{
    CAutoUnlockMutex lock(mLogMutex);

    // find the logfile with this path
    std::vector<CLogfile*>::iterator i;
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
        if (!(*i)->mPath.Compare(path))
            return (*i)->mLogMask;
    return 0;
}
void CLogManager::SetSourceFileLogMask(const char *filename, unsigned int mask)
{
    CAutoUnlockMutex lock(mLogMutex);
    mFileMasks[filename] = mask;
}
void CLogManager::ClearSourceFileLogMask(const char *filename)
{
    CAutoUnlockMutex lock(mLogMutex);
    mFileMasks.erase(filename);
}


void CLogManager::PathSetSourceFileLogMask(const char *path, const char *filename, 
                                           unsigned int mask)
{
    CAutoUnlockMutex lock(mLogMutex);
    CLogfile &logfile(getLogfile(path));
    logfile.FilterSet(CLogFilter(filename, mask));
}
void CLogManager::PathClearSourceFileLogMask(const char *path, const char *filename)
{
    CAutoUnlockMutex lock(mLogMutex);
    CLogfile &logfile(getLogfile(path));
    logfile.FilterDelete(filename);
}
void CLogManager::PathClearAllSourceFiles(const char *path)
{
    CAutoUnlockMutex lock(mLogMutex);
    CLogfile &logfile(getLogfile(path));
    logfile.FilterDeleteAll();
}

void CLogManager::PathFilterList(const char *path, std::vector<CLogFilter> &filters)
{
    CAutoUnlockMutex lock(mLogMutex);
    CLogfile &logfile(getLogfile(path));
    logfile.FilterList(filters);    
}

void CLogManager::PathList(std::vector<FString> &paths)
{
    CAutoUnlockMutex lock(mLogMutex);
    foreach (const CLogfile *logfile, mLogfiles)
        paths.push_back(logfile->mPath);
}

CLogfile & CLogManager::getLogfile(const char *path)
{
    // internal helper to get a logfile object (caller should already be locked)
    foreach (CLogfile *logfile, mLogfiles)
        if (!(logfile->mPath.Compare(path)))
            return *logfile;
    throw CForteLogException(FStringFC(), "not currently logging to '%s'", path);
}

void CLogManager::LogMsg(int level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    LogMsgVa(level, fmt, ap);
    va_end(ap);
}

void CLogManager::LogMsgVa(int level, const char *fmt, va_list ap)
{
    LogMsgVa(NULL, NULL, 0, level, fmt, ap);
}

void CLogManager::LogMsgVa(const char * func, const char * file, int line, int level, const char *fmt, va_list ap)
{
    char tmp[128];
    tmp[0] = 0;
    CLogMsg msg;
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
    CAutoUnlockMutex lock(mLogMutex);
    CLogThreadInfo *ti = (CLogThreadInfo *)mThreadInfoKey.get();
    if (ti != NULL)
        msg.mThread = &(ti->mThread);
    msg.mLevel = level;
    char *amsg;
    vasprintf(&amsg, fmt, ap);
    msg.mMsg = amsg;
    msg.mFunction = func;
    msg.mFile = file;
    msg.mLine = line;
    CLogContextStack *stack = (CLogContextStack*)mLogContextStackKey.get();
    if (stack && !(stack->mStack.empty()))
    {
        CLogContext *c(stack->mStack.back());
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
    foreach (CLogfile *i, mLogfiles)
    {
        // first check this logfile's source file filters
        bool pathFileSpecific = false;
        CLogfile &lf(*i);
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
        CLogManager *log_mgr = CLogManager::GetInstancePtr();
        if (log_mgr != NULL) log_mgr->LogMsgVa(func, file, line, level, fmt, ap);
    }
    catch (...)
    {
    }
    va_end(ap);
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
    "11",
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
FString CLogManager::LogMaskStr(int mask)
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
    return FString::join(levels, "|");
}
