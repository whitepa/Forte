#ifndef FORTE_NO_DB
#ifndef FORTE_NO_SQLITE

#include "Forte.h"
#include "DbLiteConnection.h"
#include "DbLiteResult.h"
#include "DbException.h"

DbLiteConnection::DbLiteConnection()
:
    DbConnection()
{
    m_db_type = "sqlite";
    m_db = NULL;
    m_flags = SQLITE_OPEN_READWRITE;
}


DbLiteConnection::~DbLiteConnection()
{
    Close();
}


void DbLiteConnection::set_error()
{
    if (m_db != NULL)
    {
        m_errno = sqlite3_errcode(m_db);
        m_error = sqlite3_errmsg(m_db);
    }
    else
    {
        m_errno = SQLITE_ERROR;
        m_error = "Connection is gone.";
    }
}


bool DbLiteConnection::Init(struct sqlite3 *db)
{
    m_db = db;
    m_did_init = true;
    m_reconnect = false;
    return true;
}


bool DbLiteConnection::Connect()
{
    int err;

    if (m_db != NULL && !Close()) return false;

    err = sqlite3_open_v2(m_db_name, &m_db, m_flags, NULL);

    if (err == SQLITE_OK)
    {
        sqlite3_busy_timeout(m_db, 250);
        return true;
    }

    if (m_db != NULL)
    {
        set_error();
    }
    else
    {
        m_errno = SQLITE_NOMEM;
        m_error = "Out of memory";
    }

    return false;
}


bool DbLiteConnection::Close()
{
    if (sqlite3_close(m_db) == SQLITE_OK)
    {
        m_db = NULL;
        return true;
    }

    set_error();
    return false;
}


DbResult DbLiteConnection::query(const FString& sql)
{
    unsigned int tries_remaining = m_retries + 1;
    struct timeval tv_start, tv_end;
    DbLiteResult res;
    sqlite3_stmt *stmt = NULL;
    FString remain = sql;
    const char *tail = NULL;

    // init
    m_tries = 0;
    m_errno = SQLITE_OK;
    m_error.clear();

    if (m_db == NULL)
    {
        set_error();
        return res;
    }

    if (m_autocommit == false) m_queries_pending = true;

    // while queries remain in the given sql, and we have some tries left...
    while (remain.length() && tries_remaining > 0)
    {
        // prepare statement
        m_errno = sqlite3_prepare_v2(m_db, remain, remain.length(), &stmt, &tail);
        if (stmt == NULL) break;

        do
        {
            // run query
            m_tries++;
            gettimeofday(&tv_start, NULL);
            m_errno = res.Load(stmt);
            gettimeofday(&tv_end, NULL);
            if (sDebugSql) logSql(remain, tv_end - tv_start);

            // check for errors
            if (m_errno != SQLITE_OK)
            {
                res.Clear();

                switch (m_errno)
                {
                    //// soft failures, keep retrying:
                case SQLITE_BUSY:
                case SQLITE_LOCKED:
                    sqlite3_reset(stmt);
                    --tries_remaining;
                    break;

                    //// hard failures, just fail immediately:
                default:
                    tries_remaining = 0;
                    break;
                }
            }
        }
        while (!res && tries_remaining > 0);

        // cleanup statement
        sqlite3_finalize(stmt);
        stmt = NULL;

        // set remaining sql
        if (tail == NULL) remain.clear();
        else remain = tail;
    }

    if (!res) set_error();
    return res;
}


bool DbLiteConnection::execute(const FString& sql)
{
    return query(sql);
}


DbResult DbLiteConnection::store(const FString& sql)
{
    return query(sql);
}


DbResult DbLiteConnection::use(const FString& sql)
{
    return query(sql);
}


bool DbLiteConnection::isTemporaryError() const
{
    return (m_errno == SQLITE_BUSY ||
            m_errno == SQLITE_LOCKED);
}


FString DbLiteConnection::escape(const char *str)
{
    FString ret;
    char *tmp;

    if ((tmp = sqlite3_mprintf("%q", str)) != NULL)
    {
        ret = tmp;
        sqlite3_free(tmp);
    }

    return ret;
}

#endif
#endif
