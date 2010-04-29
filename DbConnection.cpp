#ifndef FORTE_NO_DB

// DbConnection.cpp
#include "Forte.h"
#include "FTime.h"
#include "DbConnection.h"
#include "DbException.h"

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


bool DbConnection::execute2(const FString& sql)
{
    if (!execute(sql))
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", mErrno, mError.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(mError, mErrno, sql);
    }

    return true;
}


DbResult DbConnection::store2(const FString& sql)
{
    DbResult ret = store(sql);

    if (!ret)
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", mErrno, mError.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(mError, mErrno, sql);
    }

    return ret;
}


DbResult DbConnection::use2(const FString& sql)
{
    DbResult ret = use(sql);

    if (!ret)
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", mErrno, mError.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(mError, mErrno, sql);
    }

    return ret;
}


void DbConnection::debugFile(const char *filename)
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


void DbConnection::logSql(const FString &sql, const struct timeval &executionTime)
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


void DbConnection::autoCommit(bool enabled)
{
    if (mAutoCommit == false && mQueriesPending)
    {
        hlog(HLOG_ERR, "DbConnection::autoCommit() called with pending queries; committing pending queries");
        commit();
    }

    if (!enabled) begin();
    mAutoCommit = enabled;
}


void DbConnection::begin()
{
    FString sql;
    hlog(HLOG_DEBUG, "DbConnection::begin()");
    sql.Format("begin");
    execute(sql);
    mQueriesPending = true;
}


void DbConnection::commit()
{
    FString sql;
    hlog(HLOG_DEBUG, "DbConnection::commit()");
    if (mAutoCommit == true)
        hlog(HLOG_WARN, "commit() called while autocommit enabled");
    sql.Format("commit");
    execute(sql);
    mQueriesPending = false;
}


void DbConnection::rollback()
{
    FString sql;
    hlog(HLOG_DEBUG, "DbConnection::rollback()");
    if (mAutoCommit == true)
        hlog(HLOG_WARN, "rollback() called while autocommit enabled");
    sql.Format("rollback");
    execute(sql);
    mQueriesPending = false;    
}

#endif
