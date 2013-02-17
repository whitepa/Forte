#include <errno.h>
#include <string.h>
#include "Foreach.h"
#include "FTrace.h"
#include "LogManager.h"
#include "Types.h"
#include <boost/tuple/tuple.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <stdarg.h>
#include <sys/time.h>

using namespace Forte;
using namespace std;

LogManager *LogManager::sLogManager = NULL;
Mutex LogManager::sLogManagerMutex;

// Logfile
Logfile::Logfile(const FString& path, ostream *str, unsigned logMask, bool delStream,
                 unsigned int formatMask) :
    mPath(path),
    mOut(str),
    mDelStream(delStream),
    mLogMask(logMask),
    mFormatMask(formatMask)
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
    AutoUnlockMutex lock(mMutex);
    if (mOut)
    {
        (*mOut) << formatMsg(msg);
        mOut->flush();
    }
}

FString Logfile::GetLevelStr(int level)
{
    FString levelstr;

    switch (level & HLOG_ALL)
    {
    case HLOG_SQL:
        levelstr = "SQL";
        break;
    case HLOG_TRACE:
        levelstr = "TRCE";
        break;
    case HLOG_DEBUG4:
        levelstr = "DBG4";
        break;
    case HLOG_DEBUG3:
        levelstr = "DBG3";
        break;
    case HLOG_DEBUG2:
        levelstr = "DBG2";
        break;
    case HLOG_DEBUG1:
        levelstr = "DBG1";
        break;
    case HLOG_DEBUG:
        levelstr = "DBG ";
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
    AutoUnlockMutex lock(mMutex);
    mFileMasks[LogManager::SourceFileBasename(filter.mSourceFile)] = filter.mMask;
}

void Logfile::FilterDelete(const char *filename)
{
    AutoUnlockMutex lock(mMutex);
    mFileMasks.erase(LogManager::SourceFileBasename(filename));
}

void Logfile::FilterDeleteAll(void)
{
    AutoUnlockMutex lock(mMutex);
    mFileMasks.clear();
}

void Logfile::FilterList(std::vector<LogFilter> &filters)
{
    filters.clear();
    typedef std::pair<FString, unsigned int> FileMaskPair;

    AutoUnlockMutex lock(mMutex);
    foreach(const FileMaskPair &fp, mFileMasks)
        filters.push_back(LogFilter(fp.first, fp.second));
}

FString Logfile::formatMsg(const LogMsg &msg)
{
    if (mFormatMask)
    {
        return customFormatMsg(msg, mFormatMask);
    }

    FString levelstr, formattedMsg;
    struct tm lt;
    levelstr = GetLevelStr(msg.mLevel);
    localtime_r(&(msg.mTime.tv_sec), &lt);

    FString thread(FStringFC(), "[%d%s%s]", msg.mPID, (msg.mThread ?  "-" : ""), (msg.mThread ? msg.mThread->mThreadName.c_str() : ""));
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

FString Logfile::customFormatMsg(const LogMsg &msg, const int formatMask)
{
    FString formattedMsg, tmp;

    if (formatMask & HLOG_FORMAT_TIMESTAMP)
    {
        struct tm lt;
        localtime_r(&(msg.mTime.tv_sec), &lt);
        tmp.Format("%02d/%02d/%02d %02d:%02d:%02d.%03d",
                   lt.tm_mon + 1, lt.tm_mday, lt.tm_year % 100,
                   lt.tm_hour, lt.tm_min, lt.tm_sec,
                   (int)(msg.mTime.tv_usec/1000));

        formattedMsg = tmp;
    }

    if (formatMask & HLOG_FORMAT_LEVEL)
    {
        if (!formattedMsg.empty())
        {
            formattedMsg += " ";
        }
        formattedMsg += GetLevelStr(msg.mLevel);
    }

    if (formatMask & HLOG_FORMAT_THREAD)
    {
        FString thread(FStringFC(), "[%d%s%s]",
                       msg.mPID, (msg.mThread ?  "-" : ""),
                       (msg.mThread ? msg.mThread->mThreadName.c_str() : ""));
        int padT = 25-thread.size();
        if (padT<0) padT=0;
        thread.append(padT,' ');

        if (!formattedMsg.empty())
        {
            formattedMsg += " ";
        }
        formattedMsg += thread;
    }

    if (formatMask & HLOG_FORMAT_FILE)
    {
        FString fileLine(FStringFC(), "%s:%d", msg.mFile.c_str(), msg.mLine);
        int padFL = 35-fileLine.size();
        if (padFL<0) padFL=0;
        fileLine.append(padFL,' ');

        if (!formattedMsg.empty())
        {
            formattedMsg += " ";
        }
        formattedMsg += fileLine;
    }

    if (formatMask & HLOG_FORMAT_DEPTH)
    {
        if (!formattedMsg.empty())
        {
            formattedMsg += " ";
        }
        unsigned int depth = FTrace::GetDepth();
        FString offset;
        offset.append(depth * 3, ' ');
        formattedMsg += "|";
        formattedMsg += offset;
    }

    if (formatMask & HLOG_FORMAT_FUNCTION)
    {
        if (!formattedMsg.empty())
        {
            formattedMsg += " ";
        }

        formattedMsg += msg.mFunction;
        formattedMsg += "()";
    }

    if (formatMask & HLOG_FORMAT_MESSAGE)
    {
        if (!formattedMsg.empty())
        {
            formattedMsg += " ";
        }

        formattedMsg += msg.mPrefix + " " + msg.mMsg;
    }

    formattedMsg += "\n";

    return formattedMsg;
}

unsigned int Logfile::GetLogMask() const
{
    AutoUnlockMutex lock(mMutex);
    const unsigned int mask(mLogMask);
    return mask;
}

void Logfile::SetLogMask(const unsigned int& mask)
{
    AutoUnlockMutex lock(mMutex);
    mLogMask = mask;
}

unsigned int Logfile::GetFormatMask() const
{
    AutoUnlockMutex lock(mMutex);
    const unsigned int mask(mFormatMask);
    return mask;
}

void Logfile::SetFormatMask(const unsigned int& mask)
{
    AutoUnlockMutex lock(mMutex);
    mFormatMask = mask;
}

std::pair<bool, unsigned int> Logfile::GetFileMask(const Forte::FString& file) const
{
    AutoUnlockMutex lock(mMutex);

    std::map<FString,unsigned int>::const_iterator i(mFileMasks.find(file));

    if(i != mFileMasks.end())
    {
        return make_pair<bool, unsigned int>(true, i->second);
    }

    return make_pair<bool, unsigned int>(false, 0);
}

bool Logfile::IsPath(const Forte::FString& path) const
{
    return mPath == path;
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
    syslog(LOG_DAEMON | level, "%s", formatMsg(msg).c_str());
}

void SysLogfile::WriteRaw(int level, const Forte::FString& line)
{
    syslog(LOG_DAEMON | level, "%s", line.c_str());
}

FString SysLogfile::formatMsg(const LogMsg &msg)
{
    FString levelstr, formattedMsg;
    levelstr = GetLevelStr(msg.mLevel);
    formattedMsg.Format("%s [%d%s%s] %s\n",
                        levelstr.c_str(),
                        msg.mPID,
                        msg.mThread ? "-" : "",
                        msg.mThread ? msg.mThread->mThreadName.c_str() : "",
                        msg.mMsg.c_str());
    return formattedMsg;
}


// LogMsg
LogMsg::LogMsg() :
    mThread(NULL)
{
    mPID = getpid();
}

// LogThreadInfo
LogThreadInfo::LogThreadInfo(Thread &thr) : mThread(thr)
{
    AutoUnlockMutex lock(LogManager::GetSingletonMutex());
    LogManager *log_mgr = LogManager::GetInstancePtr();
    if (log_mgr != NULL)
    {
        log_mgr->mThreadInfoKey.Set(this);
    }
}

LogThreadInfo::~LogThreadInfo()
{
    // remove pointer to this log context from the manager
    AutoUnlockMutex lock(LogManager::GetSingletonMutex());
    LogManager *log_mgr = LogManager::GetInstancePtr();
    if (log_mgr != NULL)
    {
        AutoUnlockMutex lock(log_mgr->GetMutex());
        log_mgr->mThreadInfoKey.Set(NULL);
    }
}


// LogManager
LogManager::LogManager()
    :mLogMaskTemplate(HLOG_ALL),
     mLogMaskOR(HLOG_ALL)
{
    // LogManager is a singleton. There is much much here which will
    // break horribly if you declare a second LogManager object in a
    // process.  If you need a second log file, add it with a second
    // BeginLogging() call.

    AutoUnlockMutex lock(sLogManagerMutex);
    if (!sLogManager) {
        sLogManager = this;
    }
    else
        throw EGlobalLogManagerSet();
}

LogManager::~LogManager() {
    Log(HLOG_DEBUG, "logging halted");
    EndLogging();
    AutoUnlockMutex lock(sLogManagerMutex);
    sLogManager = NULL;
}

void LogManager::BeginLogging()
{
    BeginLogging("//stderr");
}

void LogManager::BeginLogging(const char *path)
{
    AutoUnlockMutex lock(mLogMutex);
    beginLogging(path, mLogMaskTemplate, HLOG_FORMAT_NONE);
}

void LogManager::BeginLogging(const char *path, int mask)
{
    AutoUnlockMutex lock(mLogMutex);
    beginLogging(path, mask, HLOG_FORMAT_NONE);
}

void LogManager::BeginLogging(const char *path, int mask, unsigned int format)
{
    AutoUnlockMutex lock(mLogMutex);
    beginLogging(path, mask, format);
}

// helper - no locking
void LogManager::beginLogging(const char *path, int mask, unsigned int format)
{
    LogfilePtr logfile;
    if (!strcmp(path, "//stderr"))
    {
        logfile = boost::make_shared<Logfile>(path, &cerr, mask, false, format);
    }
    else if (!strcmp(path, "//stdout"))
    {
        logfile = boost::make_shared<Logfile>(path, &cout, mask, false, format);
    }
    else if (!strncmp(path, "//syslog/", 9))
    {
        logfile = boost::make_shared<SysLogfile>(path + 9, mask);
    }
    else
    {
        ofstream *out = new ofstream(path, ios::app | ios::out);
        logfile = boost::make_shared<Logfile>(path, out, mask, true, format);
    }

    mLogfiles.push_back(logfile);
}

void LogManager::BeginLogging(LogfilePtr logfile)
{
    AutoUnlockMutex lock(mLogMutex);
    mLogfiles.push_back(logfile);
}

void LogManager::EndLogging()
{
    AutoUnlockMutex lock(mLogMutex);
    mLogfiles.clear();
}

void LogManager::EndLogging(const char *path)
{
    AutoUnlockMutex lock(mLogMutex);
    endLogging(path);
}

void LogManager::endLogging(const char *path)  // helper - no locking
{
    mLogfiles.erase(
        remove_if(mLogfiles.begin(), mLogfiles.end(),
                  boost::bind(&Logfile::IsPath, _1, path)),
        mLogfiles.end());
}

void LogManager::Reopen()  // re-open all log files
{
    AutoUnlockMutex lock(mLogMutex);
    typedef boost::tuple<FString,int, unsigned int> PathMaskFormatData;
    std::list<PathMaskFormatData> recreate;

    LogfileVector::iterator i;
    for (i = mLogfiles.begin(); i != mLogfiles.end(); i++)
    {
        if (!(*i)->Reopen())
        {
            recreate.push_back(
                PathMaskFormatData((*i)->mPath,
                               (*i)->GetLogMask(),
                               (*i)->GetFormatMask()));
        }
    }

    // for anything that could not reopen, re-create it
    foreach(const PathMaskFormatData &pmf, recreate)
    {
        endLogging(pmf.get<0>().c_str());
        beginLogging(pmf.get<0>(), pmf.get<1>(), pmf.get<2>());
    }

}

void LogManager::SetGlobalLogMask(unsigned int mask)
{
    AutoUnlockMutex lock(mLogMutex);
    mLogMaskTemplate = mask;
    setLogMask(0, mask);
}

void LogManager::SetLogMask(const char *path, unsigned int mask)
{
    if(path)
    {
        setLogMask(path, mask);
    }
}

void LogManager::setLogMask(const char *path, unsigned int mask)
{
    // find the logfile with this path
    LogfileVector::iterator i(mLogfiles.begin());
    LogfileVector::iterator end(mLogfiles.end());

    mLogMaskOR = mLogMaskTemplate;
    while(i != end)
    {
        if(! path || (*i)->IsPath(path))
        {
            (*i)->SetLogMask(mask);
        }

        const std::map<Forte::FString,unsigned int>& logMasks((*i)->GetLogMasks());

        for(std::map<Forte::FString,unsigned int>::const_iterator j=logMasks.begin();
            j != logMasks.end(); j++)
        {
            mLogMaskOR |= j->second;
        }

        mLogMaskOR |= (*i)->GetLogMask();

        ++i;
    }

    for(std::map<Forte::FString,unsigned int>::const_iterator k=mFileMasks.begin();
        k != mFileMasks.end(); k++)
    {
        mLogMaskOR |= k->second;
    }
}

void LogManager::setLogMaskOR()
{
    LogfileVector::iterator i(mLogfiles.begin());
    LogfileVector::iterator end(mLogfiles.end());

    mLogMaskOR = mLogMaskTemplate;
    while(i != end)
    {
        const std::map<Forte::FString,unsigned int>& logMasks((*i)->GetLogMasks());

        for(std::map<Forte::FString,unsigned int>::const_iterator j=logMasks.begin();
            j != logMasks.end(); j++)
        {
            mLogMaskOR |= j->second;
        }

        mLogMaskOR |= (*i)->GetLogMask();

        ++i;
    }

    for(std::map<Forte::FString,unsigned int>::const_iterator k=mFileMasks.begin();
        k != mFileMasks.end(); k++)
    {
        mLogMaskOR |= k->second;
    }
}

unsigned int LogManager::GetLogMask(const char *path)
{
    AutoUnlockMutex lock(mLogMutex);

    LogfileVector::iterator end(mLogfiles.end());
    LogfileVector::iterator i(find_if(mLogfiles.begin(), end, boost::bind(&Logfile::IsPath, _1, path)));

    if(i != end)
    {
        return (*i)->GetLogMask();
    }

    return 0;
}

void LogManager::SetSourceFileLogMask(const char *filename, unsigned int mask)
{
    AutoUnlockMutex lock(mLogMutex);
    mFileMasks[SourceFileBasename(filename)] = mask;
    mLogMaskOR |= mask;
}

void LogManager::ClearSourceFileLogMask(const char *filename)
{
    AutoUnlockMutex lock(mLogMutex);
    mFileMasks.erase(SourceFileBasename(filename));
    setLogMaskOR();
}

void LogManager::PathSetSourceFileLogMask(const char *path, const char *filename,
                                           unsigned int mask)
{
    AutoUnlockMutex lock(mLogMutex);
    Logfile &logfile(getLogfile(path));
    logfile.FilterSet(LogFilter(SourceFileBasename(filename), mask));
    mLogMaskOR |= mask;
}
void LogManager::PathClearSourceFileLogMask(const char *path, const char *filename)
{
    AutoUnlockMutex lock(mLogMutex);
    Logfile &logfile(getLogfile(path));
    logfile.FilterDelete(SourceFileBasename(filename));
    setLogMaskOR();
}
void LogManager::PathClearAllSourceFiles(const char *path)
{
    AutoUnlockMutex lock(mLogMutex);
    Logfile &logfile(getLogfile(path));
    logfile.FilterDeleteAll();
    setLogMaskOR();
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
    foreach (LogfileVector::value_type& logfile, mLogfiles)
        paths.push_back(logfile->mPath);
}

Logfile& LogManager::getLogfile(const char *path)
{
    LogfileVector::iterator end(mLogfiles.end());
    LogfileVector::iterator i(find_if(mLogfiles.begin(), end, boost::bind(&Logfile::IsPath, _1, path)));

    if(i != end)
    {
        return **i;
    }

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
    char *amsg(0);
    vasprintf(&amsg, fmt, ap);
    LogMsgString(func, file, line, level, amsg);
    free(amsg);
}

void LogManager::LogMsgString(const char * func, const char * fullfile, int line, int level, const std::string& message)
{
    char tmp[128];
    tmp[0] = 0;
    LogMsg msg;
    FString file(SourceFileBasename(fullfile));
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

    bool fileSpecific = false;
    LogfileWeakVector logfiles;

    {
        AutoUnlockMutex lock(mLogMutex);
        copy(mLogfiles.begin(), mLogfiles.end(), back_inserter(logfiles));

        LogThreadInfo *ti = (LogThreadInfo *)mThreadInfoKey.Get();
        if (ti != NULL)
            msg.mThread = &(ti->mThread);
        msg.mLevel = level;
        msg.mMsg = message;
        msg.mFunction = func;
        msg.mFile = file;
        msg.mLine = line;

        // decide whether this message should be logged anywhere.
        // Decision is made by (in order of precedence):
        //   Output Path: Source file specific mask
        //   Global:      Source file specific mask
        //   Output Path: Global Mask

        // check for file specific log mask
        if (mFileMasks.find(file) != mFileMasks.end())
        {
            if ((level & mFileMasks[file]) != 0)
                fileSpecific = true;
            else
                return; // do not log anywhere
        }
    }

    // send the message to all the logfiles
    foreach (LogfileWeakVector::value_type& i, logfiles)
    {
        // first check this logfile's source file filters
        LogfilePtr lf(i.lock());

        if(lf)
        {
            bool pathFileSpecific(false);

            const pair<bool, unsigned int> valuePair(lf->GetFileMask(file));

            if (valuePair.first)
            {
                if ((level & valuePair.second) != 0)
                    pathFileSpecific = true;
                else
                    continue; // don't log to this path
            }

            if (fileSpecific ||
                pathFileSpecific ||
                (level & (lf->GetLogMask())) != 0)
            {
                lf->Write(msg);
            }
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
        LogManager *log_mgr(0);
        {
            AutoUnlockMutex lock(LogManager::GetSingletonMutex());
            log_mgr = LogManager::GetInstancePtr();
        }
        if (log_mgr != NULL) log_mgr->LogMsgVa(func, file, line, level, fmt, ap);
    }
    catch (...)
    {
    }
    va_end(ap);
}

bool LogManager::ShouldLog(const char * func, const char * file, int line, int level)
{
    if(mLogMaskOR & level)
    {
        return true;
    }

    return false;
}

bool _should_hlog(const char *func, const char * file, int line, int level)
{
    try
    {
        LogManager *log_mgr(0);
        {
            AutoUnlockMutex lock(LogManager::GetSingletonMutex());
            log_mgr = LogManager::GetInstancePtr();
        }
        if (log_mgr != NULL)
        {
            return log_mgr->ShouldLog(func, file, line, level);
        }
    }
    catch (...)
    {
    }

    return false;
}

void _hlogstream(const char *func, const char * file, int line, int level, const std::string& message)
{
    try
    {
        LogManager *log_mgr(0);
        {
            AutoUnlockMutex lock(LogManager::GetSingletonMutex());
            log_mgr = LogManager::GetInstancePtr();
        }
        if (log_mgr != NULL) log_mgr->LogMsgString(func, file, line, level, message);
    }
    catch (...)
    {
    }
}

void _hlog_errno(const char* func, const char* file, int line, int level)
{
    char  err_buf[HLOG_ERRNO_BUF_LEN];

    memset(err_buf, 0, HLOG_ERRNO_BUF_LEN);
    strerror_r(errno, err_buf, HLOG_ERRNO_BUF_LEN-1);

    _hlog(func, file, line, level, "strerror: %s", err_buf);
}

bool _hlog_ratelimit(const char *file, int line, int rate)
{
    AutoUnlockMutex lock(LogManager::GetSingletonMutex());
    LogManager *log_mgr = LogManager::GetInstancePtr();
    if (log_mgr != NULL)
        return log_mgr->LogRatelimit(file, line, rate);
    else
        return true;
}

bool LogManager::LogRatelimit(const char *file, int line, int rate)
{
    FString key(FStringFC(), "%s:%d", file, line);
    time_t now = time(0);
    if (mRateLimitedTimestamps.find(key)==
        mRateLimitedTimestamps.end() ||
        mRateLimitedTimestamps[key] < now - rate)
    {
        mRateLimitedTimestamps[key] = now;
        return true;
    }
    else
        return false;
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
    "SQL",
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
int LogManager::ComputeLogMaskFromString(const FString &str) const
{
    int result = 0;
    std::vector<FString> levels;
    str.Explode("|", levels, true);
    foreach(const FString &level, levels)
        result |= GetSingleLevelFromString(level);
    return result;
}

int LogManager::GetSingleLevelFromString(const FString &str)
{
    FString tmp(str);
    tmp.MakeUpper();
    if (tmp == "ALL")
        return HLOG_ALL;
    else if (tmp == "NONE")
        return HLOG_NONE;
    else if (tmp == "NODEBUG")
        return HLOG_NODEBUG;
    else if (tmp == "MOST" || tmp == "UPTO_DEBUG3")
        return HLOG_DEBUG3 | HLOG_DEBUG2 | HLOG_DEBUG1 | HLOG_DEBUG
            | HLOG_INFO | HLOG_NOTICE | HLOG_WARN | HLOG_ERR
            | HLOG_CRIT | HLOG_ALERT | HLOG_EMERG;
    else if (tmp == "UPTO_DEBUG2")
        return HLOG_DEBUG2 | HLOG_DEBUG1 | HLOG_DEBUG
            | HLOG_INFO | HLOG_NOTICE | HLOG_WARN | HLOG_ERR
            | HLOG_CRIT | HLOG_ALERT | HLOG_EMERG;
    else if (tmp == "UPTO_DEBUG1")
        return HLOG_DEBUG1 | HLOG_DEBUG
            | HLOG_INFO | HLOG_NOTICE | HLOG_WARN | HLOG_ERR
            | HLOG_CRIT | HLOG_ALERT | HLOG_EMERG;
    else if (tmp == "UPTO_DEBUG")
        return HLOG_DEBUG
            | HLOG_INFO | HLOG_NOTICE | HLOG_WARN | HLOG_ERR
            | HLOG_CRIT | HLOG_ALERT | HLOG_EMERG;


    for (int i = 0; i < 32; i++)
    {
        if (!str.Compare(levelstr[i]))
            return 1 << i;
    }
    throw ELogLevelStringUnknown(str);
}

FString LogManager::SourceFileBasename(const FString& filename,
                                       const FString& suffix)
{
    FString result(filename);
    FStringVector pathComponents;
    int count = filename.Explode("/", pathComponents);
    if (count)
    {
        FString base = pathComponents[count-1];
        if (!suffix.empty())
        {
            FString end = base.Right(suffix.length());
            if (end.Compare(suffix) == 0)
                result = base.Left(base.length() - suffix.length());
            else
                result = base;
        }
        else
        {
            result = base;
        }
    }
    return result;
}
