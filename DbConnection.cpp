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
    mInTransaction = false;
    mErrno = 0;
    mTries = 0;
    mRetries = 20;              // default to 3 retries
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
    mInTransaction = false;

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
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", GetErrno(), GetError().c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(GetError(), GetErrno(), sql);
    }

    return true;
}


DbResult DbConnection::Store2(const FString& sql)
{
    DbResult ret = Store(sql);

    if (!ret)
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", GetErrno(), GetError().c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(GetError(), GetErrno(), sql);
    }

    return ret;
}


DbResult DbConnection::Use2(const FString& sql)
{
    DbResult ret = Use(sql);

    if (!ret)
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", GetErrno(), GetError().c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(GetError(), GetErrno(), sql);
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
    FTRACE2("%s", (enabled ? "TRUE" : "FALSE"));

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
    FTRACE2("%s/begin", mDBName.c_str());

    FString sql("begin");
    if (!Execute(sql))
    {
        FString err(FStringFC(),
                    "Begin failed: %s", GetError().c_str());
        if (IsTemporaryError())
            throw DbTempErrorException(err, GetErrno());
        else
            throw DbException(err, GetErrno());
    }
    mQueriesPending = true;
    mInTransaction = true;
}


void DbConnection::Commit()
{
    FTRACE2("%s/commit", mDBName.c_str());

    FString sql("commit");
    if (mAutoCommit == true)
        hlog(HLOG_WARN, "commit() called while autocommit enabled");

    if (!Execute(sql))
    {
        FString err(FStringFC(),
                    "Commit failed: %s", GetError().c_str());
        if (IsTemporaryError())
            throw DbTempErrorException(err, GetErrno());
        else
            throw DbException(err, GetErrno());
    }

    mQueriesPending = false;
    mInTransaction = false;
}


void DbConnection::Rollback()
{
    FTRACE;

    FString sql("rollback");
    if (mAutoCommit == true)
        hlog(HLOG_WARN, "rollback() called while autocommit enabled");

    if (!Execute(sql))
    {
        FString err(FStringFC(),
                    "Rollback failed: %s", GetError().c_str());
        if (IsTemporaryError())
            throw DbTempErrorException(err, GetErrno());
        else
            throw DbException(err, GetErrno());
    }

    mQueriesPending = false;
    mInTransaction = false;
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

FString DbConnection::GetError() const
{
    return mError;
}

unsigned int DbConnection::GetErrno() const
{
    return mErrno;
}

unsigned int DbConnection::GetTries() const
{
    return mTries;
}

unsigned int DbConnection::GetRetries() const
{
    return mRetries;
}

unsigned int DbConnection::GetQueryRetryDelay() const
{
    return mQueryRetryDelay;
}

#endif
