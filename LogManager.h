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
#include <boost/throw_exception.hpp>

#define HLOG_NONE        0x00000000
#define HLOG_SQL         0x00000400
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

#define HLOG_MIN         HLOG_SQL
#define HLOG_MAX         HLOG_EMERG
#define HLOG_WARNING     HLOG_WARN
#define HLOG_ERROR       HLOG_ERR
#define HLOG_DEBUG0      HLOG_DEBUG

#define HLOG_ERRNO_BUF_LEN    256

#define HLOG_FORMAT_NONE        0x00000000 // use default FormatMsg
#define HLOG_FORMAT_TIMESTAMP   0x00000001
#define HLOG_FORMAT_LEVEL       0x00000002
#define HLOG_FORMAT_THREAD      0x00000004
#define HLOG_FORMAT_FILE        0x00000008
#define HLOG_FORMAT_DEPTH       0x00000010
#define HLOG_FORMAT_FUNCTION    0x00000020
#define HLOG_FORMAT_MESSAGE     0x00000040
#define HLOG_FORMAT_ALL         0x00FFFFFF
#define HLOG_FORMAT_SIMPLE      HLOG_FORMAT_TIMESTAMP | HLOG_FORMAT_LEVEL \
                                | HLOG_FORMAT_FILE | HLOG_FORMAT_MESSAGE


namespace Forte
{
    EXCEPTION_CLASS(ELog);
    EXCEPTION_SUBCLASS2(ELog, ELogLevelStringUnknown, "Log level string is unknown");
    EXCEPTION_SUBCLASS2(ELog, EGlobalLogManagerSet, "A global log manager is already selected");

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
        int mPID;
        struct in_addr mClient;
        FString mPrefix;
        FString mMsg;
    };

    class LogFilter : public Object {
    public:
        LogFilter(const FString sourceFile, int mask) :
            mSourceFile(sourceFile), mMask(mask), mMode(0) {};
        LogFilter(const LogFilter &other) {
            *this = other;
        }
        const LogFilter & operator= (const LogFilter &rhs) {
            mSourceFile = rhs.mSourceFile;
            mMask = rhs.mMask;
            mMode = rhs.mMode;
            return *this;
        }


        FString mSourceFile;
        int mMask;
        int mMode; ///< FUTURE USE:  include (only) / exclude
    };

    class Logfile : public Object {
    public:
        Logfile(const FString &path,
                std::ostream *stream,
                unsigned int logMask = HLOG_ALL,
                bool delStream = false,
                unsigned int formatMask = HLOG_FORMAT_NONE);
        virtual ~Logfile();
        FString mPath;
        std::ostream *mOut;
        bool mDelStream;
        unsigned mLogMask; // bitmask of desired log levels
        unsigned int mFormatMask; // bitmask of desired log fields
        // source file specific log masks
        std::map<FString,unsigned int> mFileMasks;

        void FilterSet(const LogFilter &filter);
        void FilterDelete(const char *filename);
        void FilterDeleteAll(void);
        void FilterList(std::vector<LogFilter> &filters);

        virtual void Write(const LogMsg& msg);
        virtual FString FormatMsg(const LogMsg &msg);
        virtual FString CustomFormatMsg(const LogMsg &msg);
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
    class LogThreadInfo : public Object {
        friend class LogManager;
    public:
        LogThreadInfo(Thread &thr);
        virtual ~LogThreadInfo();

    protected:
        Thread &mThread;
    };
    /**
     *The Forte LogManager allows you to control what log messages from an
     *application are logged and where those messages are logged.
     *The BeginLogging() function controls where messages are logged.
     *You can call BeginLogging() for as many log outputs as you need.
     *The EndLogging() function allows you to stop logging messages on a specific log
     *output file or all of them. Logging of messages is controlled by two filters,
     *a source file filter and a log level filter. The source file filter allows you
     *to select what file you will log messages for.
     *
     *The log level filter allows you to choose what types of messages you will log using a bit mask that
     *specifies all levels to be included in the specific log file. There are 15 possible log levels:
     *
     *HLOG_TRACE - Traces in and out of the function (used only by FTRACE).
     *
     *HLOG_DEBUG1 through 4 - used to represent different priorities of error messages. Four is most detailed
     *and one is least detailed.
     *
     *HLOG_DEBUG - Used for logging general debugging messages. This is the most commonly used debug option.
     *
     *HLOG_INFO - Information you want printed to the user in case they want to know what is going on.
     *
     *HLOG_NOTICE - For logging the lowest level of error message. It is for errors you might notice, but they
     *are not significant.
     *
     *HLOG_WARN - For logging warning error messages. It is for errors you do notice and which might be
     *problematic.
     *
     *HLOG_ERR - For logging error messages. It is for errors you do notice which pose a problem that you
     *want to correct.
     *
     *HLOG_CRIT - For logging critical error messages. It is for errors which are somewhat severe and require
     *attention.
     *
     *HLOG_ALERT - For logging alert level error messages. It is for errors which are severe and require
     *immediate attention.
     *
     *HLOG_EMERG - For logging emergency level error messages. These are for the worst errors that absolutely
     *require attention.
     *
     *HLOG_ALL - For logging all possible types of error messages.
     *
     *HLOG_NODEBUG - For logging everything but debug messages.
     *
     *HLOG_MIN - The same as HLOG_TRACE.
     *
     *HLOG_MAX - The same as HLOG_EMERG.
     *
     *HLOG_WARNING - The same as HLOG_WARN.
     *
     *HLOG_ERROR - The same as HLOG_ERR.
     *
     *HLOG_DEBUG0 - The same as HLOG_DEBUG.
     **/
    class LogManager : public Object {
        friend class LogThreadInfo;
    public:
        /**
         *LogManager() is used to create a LogManager object. After creating this object, typically you
         *will call a variation of BeginLogging() and give it a filepath to start logging information for.
         **/
        LogManager();
        virtual ~LogManager();

        /**
         * NOTE: you MUST hold the sLogManagerMutex prior to calling these!
         */
        inline static LogManager* GetInstancePtr() { return sLogManager; }
        inline static LogManager & GetInstance() {
            if (sLogManager) return *sLogManager; else throw EEmptyReference("no log manager instance");
        }

        /**
         *BeginLogging() permits you to start logging error messages to //stderr.
         *The types of messages logged are specified by SetGlobalLogMask. If
         *SetGlobalLogMask does not specify anything, by default all types of
         *error messages are logged. You can call BeginLogging() as many times as you want.
         **/

        void BeginLogging(); // start logging on stderr

        /**
         *BeginLogging(const char *path) takes a file path you want to log to. Special cases
         *for the path can be //stdout or //stderr. The types of error messages logged
         *are specified by SetGlobalLogMask. If SetGlobalLogMask does not
         *specify anything, by default all types of error messages are logged. You can
         *call BeginLogging(const char *path) as many times as you want.
         **/

        void BeginLogging(const char *path);  // log to a file, can be '//stdout' or '//stderr'

        /**
         *BeginLogging(const char *path, int mask) takes a file path to log to and
         *the types of error messages you want to log. For more information about
         *types of messages you can log see the list of defined int mask options.
         *Special cases for the selected file path can be //stdout or //stderr.
         * You can call BeginLogging(const char *path, int mask) as many times as you want.
         **/

        void BeginLogging(const char *path, int mask);  // log to a file, can be '//stdout' or '//stderr'

        // log to a file using a combination of the HLOG_FORMAT*
        // columns. file can be '//stdout' or '//stderr'
        void BeginLogging(const char *path, int mask, unsigned int format);

        /**
         *BeginLogging(Logfile *logfile) is a shortcut version of the BeginLogging()
         *function. Logfile is a class that describes a file path and messages to log
         *for you. You can call BeginLogging(Logfile *logfile) as many times as you want.
         **/

        void BeginLogging(Logfile *logfile); // log to a custom Logfile object; takes ownership

        /**
         *EndLogging() ends all logging on everything you called any variation of
         *BeginLogging() on.
         **/

        void EndLogging();   // stop all logging

        /**
         *EndLogging(const char *path) ends all logging in a specified logfile.
         **/

        void EndLogging(const char *path); // stop logging to a specific location

        /**
         *ReOpen() reopens logging on each file you called any variation of BeginLogging on.
         **/
        void Reopen();  // re-open all log files

        /**
         *SetLogMask(const char *path, unsigned int mask) takes a path to a logfile
         *you want to log error messages in and an unsigned int mask which represents
         *all the types of error messages you want to log. The next time you call
         *the BeginLogging() function it will use the settings you select here.
         **/
        void SetLogMask(const char *path, unsigned int mask); // bitmask of desired log levels
        unsigned int GetLogMask(const char *path);

        /**
         *SetGlobalLogMask(unsigned int mask) takes an unsigned int mask representing
         *different levels of error messages. The list of error messages you provide
         *will replace the settings for what error messages are being logged in every
         *logfile you’ve run a variation of BeginLogging on. For the next log that
         *comes in, these new settings will be used. Calling this function also resets
         *the default setting to be whatever it is you have specified. For example,
         *if you call a function such as BeginLogging and do not provide any arguments,
         *the default will match whatever you have set using
         *SetGlobalLogMask(unsigned int mask).
         **/

        void SetGlobalLogMask(unsigned int mask); // set log mask on ALL log files

        /**
         *SetSourceFileLogMask(const char *filename, unsigned int mask) takes the
         *name of a source file and a list of levels of error messages you want
         *recorded for that source file. It replaces the existing info (if any)
         *regarding this source file you may have been logging for rather than
         *adding to it.  This works globally, so all log files you called a variation
         *of BeginLogging on will now have their levels reset to match whatever you
         *choose here.
         **/

        void SetSourceFileLogMask(const char *filename, unsigned int mask);

        /**
         *ClearSourceFileLogMask(const char filename) takes a filename and clears
         *the logging information for the source file specified but leaves other
         *logging information for any other source files in the logfile untouched.
         **/

        void ClearSourceFileLogMask(const char *filename);

        /**
         *PathSetSourceFileLogMask(const char *path, const char *filename, unsigned int mask)
         *writes levels of error messages you choose for a source file you choose for a
         *log file that you choose.
         **/

        void PathSetSourceFileLogMask(const char *path, const char *filename, unsigned int mask);

        /**
         *PathClearSourceFileLogMask(const char *path, const char *filename, unsigned int mask)
         *works the same as ClearSourceFileLogMask but clears all levels of error
         *messages for a source file in a specific log file.
         **/

        void PathClearSourceFileLogMask(const char *path, const char *filename);

        /**
         *PathClearAllSourceFiles(const char *path) takes the path to a specific
         *logfile and clears all logging for source files in that specific logfile.x
         **/

        void PathClearAllSourceFiles(const char *path);

        /**
         *PathFilterList(const char *path, std:vector<LogFilter> &filters) returns
         *a list of all source files and the error  levels its logged at in the
         *logfile specified.
         **/

        void PathFilterList(const char *path, std::vector<LogFilter> &filters);

        /**
         *PathList(std::vector<FString> &paths) returns a list of every path you
         *called a variation of BeginLogging on.
         **/
        void PathList(std::vector<FString> &paths);

        /**
         * Compute the basename of the filename given. This is needed
         * as the __FILE__ macro often includes the full path to
         * header files (if hlog() is called from within a header).
         */
        static FString SourceFileBasename(const FString& filename,
                                          const FString& suffix = "");

        /**
         *Log(int level, const char *fmt,…) attribute((format(printf
         *lets you log a message at one of many levels error (choose from the complete
         *list of defined HLOG values). It goes through every log you called a variation
         *of BeginLogging on and asks if it should write messages of a certain level to
         *that file. If yes, it writes the messages to that file.
         **/

        void Log(int level, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

        /**
         *LogMsgVa(int level, const char *fmt , va_list ap) is only for specific cases
         *where you have a variable argument list. This performs the same function as
         *Log(). It is just a different way to call it.
         **/

        void LogMsgVa(int level, const char *fmt, va_list ap);

        /**
         *LogMsgVa(const char *func, const char *file, int line, int level, const char *fmt, va_list ap)
         *performs the same function as Log() but it is a special case where
         *it can take in more information. This function is only used by the trace (FTrace) routine.
         **/

        void LogMsgVa(const char * func, const char *file, int line, int level, const char *fmt, va_list ap);

        /**
         *LogMsgString(const char *func, const char *file, int line, int level, const std::string& message)
         *performs the same function as Log() but it is a special case where it takes a std::string as a message
         **/

        void LogMsgString(const char * func, const char * file, int line, int level, const std::string& message);

        static FString LogMaskStr(int mask);

        /**
         * ComputeLogMaskFromString will take the given string and
         * return an integer log mask.  Bitwise OR of the log levels
         * is supported, for example, supplying the string
         * 'INFO | ERR | WARN' will return the appropriate log mask to see
         * log messages of type INFO, ERR, and WARN.
         */
        int ComputeLogMaskFromString(const FString &str) const;

        /**
         * GetSingleLevelFromString will return the value of the
         * single log level represented by the given string. If the
         * string is invalid, ELogLevelStringUnknown will be thrown.
         * Special log levels of "ALL", "NODEBUG", and "MOST" are also
         * supported.
         */
        int GetSingleLevelFromString(const FString &str) const;

        /**
         * Get a reference to the log mutex to allow external locking
         * of the LogManager.
         */
        Mutex& GetMutex(void) { return mLogMutex; }

        /*
         * Get a reference to the log singleton mutex to allow locking from
         * hlog()
         */
        static Mutex& GetSingletonMutex(void) { return sLogManagerMutex; }


        /**
         * LogRatelimit() will limit the logging from a particular
         * file/line to once every given number of seconds.  Call this
         * method prior to logging any message you need ratelimited.
         * If this method returns true, then call hlog().
         *
         * @param rate logging will be limited to once every 'rate'
         * seconds
         * @return bool whether or not caller should call hlog()
         */
        bool LogRatelimit(const char *file, int line, int rate);

    protected:
        friend class Mutex;
        std::vector<Logfile*> mLogfiles;
        ThreadKey mThreadInfoKey;
        Mutex mLogMutex;
        unsigned mLogMaskTemplate;
        static LogManager *sLogManager;
        static Mutex sLogManagerMutex;

        // Source File specific log masks (global)
        std::map<FString,unsigned int> mFileMasks;

        std::map<FString,time_t> mRateLimitedTimestamps;

        void beginLogging(const char *path, int mask, unsigned int format); // helper - no locking
        void endLogging(const char *path);    // helper - no locking

        Logfile &getLogfile(const char *path); // get logfile object for a given path
    };
};
// hermes API compatibility:
//void hlog(int level, const char* fmt, ...);

void _hlog(const char *func, const char *file, int line, int level, const char * fmt, ...) __attribute__((format(printf, 5, 6)));
#define hlog(level, fmt...) _hlog(__FUNCTION__, __FILE__, __LINE__, level, fmt)

void _hlogstream(const char *func, const char *file, int line, int level, const std::string& message);
#define hlogstream(level, message) {                                    \
        std::ostringstream o;                                           \
        o << message;                                                   \
        _hlogstream(__FUNCTION__, __FILE__, __LINE__, level, o.str()); }

void _hlog_errno(const char* func, const char* file, int line, int level);
#define hlog_errno(level)  _hlog_errno(__FUNCTION__, __FILE__, __LINE__, level)

#define hlog_and_throw(level, exception_decl)                           \
    {                                                                   \
        const std::exception &exception_instance = exception_decl;      \
        hlog(level, "EXCEPTION %s thrown (%s)",                         \
             typeid(exception_instance).name(),                         \
             exception_decl.what());                                    \
        boost::throw_exception(exception_decl);                         \
    }


/**
 * hlog_ratelimit() will limit the logging from a particular file/line
 * to once every given number of seconds.
 */
#define hlog_ratelimit(rate) _hlog_ratelimit(__FILE__, __LINE__, rate)
bool _hlog_ratelimit(const char *file, int line, int rate);

#endif
