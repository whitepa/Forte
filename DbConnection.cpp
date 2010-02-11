#ifndef FORTE_NO_DB

// DbConnection.cpp
#include "Forte.h"
#include "FTime.h"
#include "DbConnection.h"
#include "DbException.h"

ofstream CDbConnection::sDebugOutputFile;
CMutex CDbConnection::sDebugOutputMutex;
bool CDbConnection::sDebugSql = false;


CDbConnection::CDbConnection()
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


CDbConnection::~CDbConnection()
{
}


bool CDbConnection::Init(const FString& db, const FString& user, const FString& pass,
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


bool CDbConnection::execute2(const FString& sql)
{
    if (!execute(sql))
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", m_errno, m_error.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw CDbException(m_error, m_errno, sql);
    }

    return true;
}


CDbResult CDbConnection::store2(const FString& sql)
{
    CDbResult ret = store(sql);

    if (!ret)
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", m_errno, m_error.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw CDbException(m_error, m_errno, sql);
    }

    return ret;
}


CDbResult CDbConnection::use2(const FString& sql)
{
    CDbResult ret = use(sql);

    if (!ret)
    {
        hlog(HLOG_DEBUG, "DB Error: [%u] %s", m_errno, m_error.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        throw CDbException(m_error, m_errno, sql);
    }

    return ret;
}


void CDbConnection::debugFile(const char *filename)
{
    CAutoUnlockMutex lock(sDebugOutputMutex);

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


void CDbConnection::logSql(const FString &sql, const struct timeval &executionTime)
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
                   CTime::to_str(tv.tv_sec, "US/Pacific").c_str(), 
                   (unsigned)tv.tv_usec / 1000, (unsigned)pthread_self(), sql.c_str());
#endif
    CAutoUnlockMutex lock(sDebugOutputMutex);
    if (sDebugSql) sDebugOutputFile << logMsg << endl;
}


void CDbConnection::autoCommit(bool enabled)
{
    if (m_autocommit == false && m_queries_pending)
    {
        hlog(HLOG_ERR, "CDbConnection::autoCommit() called with pending queries; committing pending queries");
        commit();
    }

    if (!enabled) begin();
    m_autocommit = enabled;
}


void CDbConnection::begin()
{
    FString sql;
    hlog(HLOG_DEBUG, "CDbConnection::begin()");
    sql.Format("begin");
    execute(sql);
    m_queries_pending = true;
}


void CDbConnection::commit()
{
    FString sql;
    hlog(HLOG_DEBUG, "CDbConnection::commit()");
    if (m_autocommit == true)
        hlog(HLOG_WARN, "commit() called while autocommit enabled");
    sql.Format("commit");
    execute(sql);
    m_queries_pending = false;
}


void CDbConnection::rollback()
{
    FString sql;
    hlog(HLOG_DEBUG, "CDbConnection::rollback()");
    if (m_autocommit == true)
        hlog(HLOG_WARN, "rollback() called while autocommit enabled");
    sql.Format("rollback");
    execute(sql);
    m_queries_pending = false;    
}

#endif
