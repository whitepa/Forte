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
    m_did_init = false;
    m_reconnect = false;
    m_queries_pending = false;
    m_autocommit = true;
    m_log_queries = false;

    m_errno = 0;
    m_tries = 0;

    m_retries = 3;              // default to 3 retries
    m_query_retry_delay = 1000; // 1 second delay
}


DbConnection::~DbConnection()
{
}


bool DbConnection::Init(const FString& db, const FString& user, const FString& pass,
                         const FString& host, const FString& socket, unsigned int retries)
{
    if (m_did_init) return true;

    m_db_name = db;
    m_host = host;
    m_user = user;
    m_password = pass;
    m_socket = socket;
    m_retries = retries;

    if (Connect())
    {
        m_did_init = true;
        // we have been initialized with information sufficient to reconnect if needed
        m_reconnect = true;
    }

    return m_did_init;
}


bool DbConnection::execute2(const FString& sql)
{
    if (!execute(sql))
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", m_errno, m_error.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(m_error, m_errno, sql);
    }

    return true;
}


DbResult DbConnection::store2(const FString& sql)
{
    DbResult ret = store(sql);

    if (!ret)
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", m_errno, m_error.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(m_error, m_errno, sql);
    }

    return ret;
}


DbResult DbConnection::use2(const FString& sql)
{
    DbResult ret = use(sql);

    if (!ret)
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", m_errno, m_error.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw DbException(m_error, m_errno, sql);
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
    if (m_autocommit == false && m_queries_pending)
    {
        hlog(HLOG_ERR, "DbConnection::autoCommit() called with pending queries; committing pending queries");
        commit();
    }

    if (!enabled) begin();
    m_autocommit = enabled;
}


void DbConnection::begin()
{
    FString sql;
    hlog(HLOG_DEBUG, "DbConnection::begin()");
    sql.Format("begin");
    execute(sql);
    m_queries_pending = true;
}


void DbConnection::commit()
{
    FString sql;
    hlog(HLOG_DEBUG, "DbConnection::commit()");
    if (m_autocommit == true)
        hlog(HLOG_WARN, "commit() called while autocommit enabled");
    sql.Format("commit");
    execute(sql);
    m_queries_pending = false;
}


void DbConnection::rollback()
{
    FString sql;
    hlog(HLOG_DEBUG, "DbConnection::rollback()");
    if (m_autocommit == true)
        hlog(HLOG_WARN, "rollback() called while autocommit enabled");
    sql.Format("rollback");
    execute(sql);
    m_queries_pending = false;    
}

#endif
