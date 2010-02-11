#ifndef FORTE_NO_DB
#ifndef FORTE_NO_POSTGRESQL

// CDbPgConnection.cpp
#include "Forte.h"
#include "DbPgConnection.h"
#include "DbPgResult.h"
#include "DbException.h"
#include "AutoMutex.h"

CDbPgConnection::CDbPgConnection()
:
    CDbConnection()
{
    m_db = NULL;
    m_db_type = "postgresql";
}


CDbPgConnection::~CDbPgConnection()
{
    Close();
}


bool CDbPgConnection::Init(PGconn *db)
{
    m_db = db;
    m_did_init = true;
    m_reconnect = false;
    return true;
}


bool CDbPgConnection::Connect(void)
{
    bool ret;
    unsigned int tries = m_retries + 1;
    FString conn_info, stmp;
    vector<FString> params, parts;

    // build parameters
    if (!m_socket.empty())
    {
        stmp.Format("host='%s'", escape(m_socket).c_str());
    }
    else
    {
        parts = m_host.split(":");
        m_host = parts[0];
        stmp.Format("host='%s'", escape(m_host).c_str());

        if (parts.size() > 1)
        {
            params.push_back(stmp);
            stmp.Format("port=%u", parts[1].asUnsignedInteger());
        }
    }

    params.push_back(stmp);

    stmp.Format("dbname='%s'", escape(m_db_name).c_str());
    params.push_back(stmp);

    stmp.Format("user='%s'", escape(m_user).c_str());
    params.push_back(stmp);

    stmp.Format("password='%s'", escape(m_password).c_str());
    params.push_back(stmp);

    // build the conn_info string
    conn_info = FString::join(params, "\n");

    // connect to the server
    do
    {
        Close();
        m_db = PQconnectdb(conn_info);
        ret = (PQstatus(m_db) == CONNECTION_OK);

        if (!ret)
        {
            m_errno = PQstatus(m_db) + 1000;
            --tries;
        }

        if (!ret && tries > 0) usleep(m_query_retry_delay * 1000);
    }
    while (!ret && tries > 0);

    if (!ret)
    {
        m_error = PQerrorMessage(m_db);
        Close();
    }

    return ret;
}


bool CDbPgConnection::Close(void)
{
    if (m_db != NULL) PQfinish(m_db);
    m_db = NULL;
    return true;
}


CDbResult CDbPgConnection::query(const FString& sql)
{
    unsigned int tries_remaining = m_retries + 1;
    struct timeval tv_start, tv_end;
    CDbPgResult res;
    int code;

    // init
    m_tries = 0;
    m_errno = 0;
    m_error.clear();

    if (m_db == NULL)
    {
        m_error.assign("Connection is gone.");
        m_errno = CONNECTION_BAD + 1000;
        m_last_res = res;
        return res;
    }

    if (m_autocommit == false) m_queries_pending = true;

    do
    {
        // run query
        m_tries++;
        gettimeofday(&tv_start, NULL);
        res = PQexec(m_db, sql);
        gettimeofday(&tv_end, NULL);
        if (sDebugSql) logSql(sql, tv_end - tv_start);
        if ((PGresult*)res == NULL) code = CONNECTION_BAD + 1000;
        else code = PQresultStatus(res);

        // check for errors
        if (!res.isOkay() ||
            ((code != PGRES_EMPTY_QUERY) &&
             (code != PGRES_COMMAND_OK) &&
             (code != PGRES_TUPLES_OK)))
        {
            m_errno = code;
            res.Clear();

            switch (m_errno)
            {
            //// soft failures, keep retrying:
            case (CONNECTION_BAD + 1000):
            case PGRES_BAD_RESPONSE:
                if (m_queries_pending)
                    throw CDbException("connection lost during transaction", m_errno, sql);
                // try reconnecting
                Connect();
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

    m_last_res = res;

    if (m_db == NULL)
    {
        m_error.assign("Connection lost; unable to reestablish.");
        return res;
    }

    if (!res) m_error = PQerrorMessage(m_db);
    return res;
}


bool CDbPgConnection::execute(const FString& sql)
{
    return query(sql);
}


CDbResult CDbPgConnection::store(const FString& sql)
{
    return query(sql);
}


CDbResult CDbPgConnection::use(const FString& sql)
{
    return query(sql);
}


bool CDbPgConnection::isTemporaryError() const
{
    return (m_errno == PGRES_COPY_OUT ||
            m_errno == PGRES_COPY_IN);
}


FString CDbPgConnection::escape(const char *str)
{
    FString ret, sql(str);;
    char *foo = new char[2 + 2 * sql.length()];

    if (m_db != NULL)
    {
        // use correct method
        PQescapeStringConn(m_db, foo, sql, sql.length(), NULL);
    }
    else
    {
        // use deprecated function
        static CMutex mutex;
        CAutoUnlockMutex lock(mutex);
        PQescapeString(foo, sql, sql.length());
    }

    ret = foo;
    delete [] foo;
    return ret;
}

#endif
#endif
