#ifndef FORTE_NO_DB

#include "FTime.h"
#include "FTrace.h"
#include "DbConnection.h"
#include "DbException.h"
#include "DbSqlStatement.h"
#include "LogManager.h"

using namespace Forte;

ofstream DbConnection::sDebugOutputFile;
Mutex DbConnection::sDebugOutputMutex;
bool DbConnection::sDebugSql = false;


DbConnection::DbConnection()
{
    mDidInit = false;
    mReconnect = false;
    mQueriesPending = false;
    mAutoCommit = true;
    mLogQueries = false;

    mErrno = 0;
    mTries = 0;

    mRetries = 3;              // default to 3 retries
    mQueryRetryDelay = 1000; // 1 second delay
}


DbConnection::~DbConnection()
{
}


bool DbConnection::Init(const FString& db, const FString& user, const FString& pass,
                         const FString& host, const FString& socket, unsigned int retries)
{
    if (mDidInit) return true;

    mDBName = db;
    mHost = host;
    mUser = user;
    mPassword = pass;
    mSocket = socket;
    mRetries = retries;

    if (Connect())
    {
        mDidInit = true;
        // we have been initialized with information sufficient to reconnect if needed
        mReconnect = true;
    }

    return mDidInit;
}


bool DbConnection::Execute2(const FString& sql)
{
    if (!Execute(sql))
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", mErrno, mError.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(mError, mErrno, sql);
    }

    return true;
}


DbResult DbConnection::Store2(const FString& sql)
{
    DbResult ret = Store(sql);

    if (!ret)
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", mErrno, mError.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(mError, mErrno, sql);
    }

    return ret;
}


DbResult DbConnection::Use2(const FString& sql)
{
    DbResult ret = Use(sql);

    if (!ret)
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", mErrno, mError.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(mError, mErrno, sql);
    }

    return ret;
}


void DbConnection::DebugFile(const char *filename)
{
    AutoUnlockMutex lock(sDebugOutputMutex);

    if (sDebugSql) sDebugOutputFile.close();

    if (filename == NULL)
    {
        sDebugSql = false;
        return;
    }

    sDebugOutputFile.open(filename);

    if (sDebugOutputFile.good())
    {
        hlog(HLOG_INFO, "SQL debugging is enabled to output file '%s'", filename);
        sDebugSql = true;
    }
    else
    {
        hlog(HLOG_ERR, "failed to open SQL debug output file '%s'", filename);
    }
}


void DbConnection::LogSql(const FString &sql, const struct timeval &executionTime)
{
#if defined(FORTE_NO_BOOST) || !defined(FORTE_WITH_DATETIME)
    FString logMsg(FStringFC(), "%04u.%06u [%u]: %s", 
                   (unsigned)executionTime.tv_sec, (unsigned)executionTime.tv_usec,
                   (unsigned)pthread_self(), sql.c_str());
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    FString logMsg(FStringFC(), "%04u.%06u %s.%03u [%u]: %s", 
                   (unsigned)executionTime.tv_sec, (unsigned)executionTime.tv_usec, 
                   FTime::to_str(tv.tv_sec, "US/Pacific").c_str(), 
                   (unsigned)tv.tv_usec / 1000, (unsigned)pthread_self(), sql.c_str());
#endif
    AutoUnlockMutex lock(sDebugOutputMutex);
    if (sDebugSql) sDebugOutputFile << logMsg << endl;
}


void DbConnection::AutoCommit(bool enabled)
{
    if (mAutoCommit == false && mQueriesPending)
    {
        hlog(HLOG_ERR, "DbConnection::AutoCommit() called with pending queries; committing pending queries");
        Commit();
    }

    if (!enabled) Begin();
    mAutoCommit = enabled;
}


void DbConnection::Begin()
{
    FTRACE;

    FString sql;
    sql.Format("begin");
    Execute(sql);
    mQueriesPending = true;
}


void DbConnection::Commit()
{
    FTRACE;

    FString sql;
    if (mAutoCommit == true)
        hlog(HLOG_WARN, "commit() called while autocommit enabled");
    sql.Format("commit");
    Execute(sql);
    mQueriesPending = false;
}


void DbConnection::Rollback()
{
    FTRACE;

    FString sql;
    if (mAutoCommit == true)
        hlog(HLOG_WARN, "rollback() called while autocommit enabled");
    sql.Format("rollback");
    Execute(sql);
    mQueriesPending = false;    
}

void DbConnection::BackupDatabase(const FString &targetPath)
{
    throw EDbConnectionNotImplemented();
}

bool DbConnection::Execute(const DbSqlStatement& statement)
{
    return Execute(statement.GetStatement());
}

DbResult DbConnection::Use(const DbSqlStatement& statement)
{
    return Use(statement.GetStatement());
}

DbResult DbConnection::Store(const DbSqlStatement& statement)
{
    return Store(statement.GetStatement());
}

const std::string& DbConnection::GetDbName() const
{
    return mDBName;
}

bool DbConnection::HasPendingQueries() const
{ 
    return mQueriesPending;
}

#endif
