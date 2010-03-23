#ifndef FORTE_NO_DB
#ifndef FORTE_NO_MYSQL

#include "Forte.h"
#include "DbMyConnection.h"
#include "DbMyResult.h"
#include "DbException.h"

DbMyConnection::DbMyConnection()
:
    DbConnection()
{
    mysql_init(&m_mysql);
    m_db_type = "mysql";
    m_db = NULL;
}


DbMyConnection::DbMyConnection(const MySQLOptionArray& options)
:
    DbConnection()
{
    unsigned i, n = options.size();

    // basic init
    mysql_init(&m_mysql);
    m_db_type = "mysql";
    m_db = NULL;
    
    // plus des options
    for (i=0; i<n; i++)
    {
        mysql_options(&m_mysql, options[i].first, options[i].second);
    }
}


DbMyConnection::~DbMyConnection()
{
    Close();
}


bool DbMyConnection::Init(MYSQL *mysql)
{
    m_db = mysql;
    m_did_init = true;
    m_reconnect = false;
    return true;
}


bool DbMyConnection::Connect(void)
{
    bool ret;
    unsigned int tries = m_retries + 1;

    do
    {
        const char *sock = NULL;
        if (m_socket.length() > 0) sock = m_socket.c_str();
        m_db = mysql_real_connect(&m_mysql, m_host, m_user, m_password,
                                  m_db_name, 0, sock, 0);
        ret = (m_db != NULL);

        if (!ret)
        {
            m_errno = mysql_errno(&m_mysql);

            switch (m_errno)
            {
            //// soft failures, keep retrying:
            case ER_CON_COUNT_ERROR:
            case CR_CONNECTION_ERROR:
            case CR_CONN_HOST_ERROR:
            case CR_SERVER_GONE_ERROR:
            case CR_SERVER_LOST:
                --tries;
                break;
            //// hard failures, just fail immediately:
            default:
                tries = 0;
                break;
            }
        }

        if (!ret && tries > 0) usleep(m_query_retry_delay * 1000);
    }
    while (!ret && tries > 0);

    if (!ret) m_error = mysql_error(&m_mysql);
    return ret;
}


bool DbMyConnection::Close(void)
{
    if (m_db != NULL) mysql_close(m_db);
    m_db = NULL;
    return true;
}


bool DbMyConnection::execute(const FString& sql)
{
    bool ret;
    unsigned int tries_remaining = m_retries + 1;
    struct timeval tv_start, tv_end;

    // init
    m_tries = 0;
    m_errno = 0;
    m_error.clear();

    if (m_db == NULL && !Connect()) // try to repair the connection
    {
        m_error.assign("NULL database handle in DbMyConnection::execute()");
        return false;
    }

    if (m_autocommit == false) m_queries_pending = true;

    do
    {
        // run query
        m_tries++;
        gettimeofday(&tv_start, NULL);
        if (m_db == NULL)
        {
            m_error.assign("NULL database handle before query");
            return false;
        }
        ret = mysql_real_query(m_db, sql, sql.length());
        gettimeofday(&tv_end, NULL);
        if (sDebugSql) logSql(sql, tv_end - tv_start);

        // check for errors
        if (ret)
        {
            m_errno = mysql_errno(m_db);

            switch(m_errno)
            {
            //// soft failures, keep retrying:
            case CR_SERVER_GONE_ERROR:
            case CR_SERVER_LOST:
                if (m_queries_pending)
                    throw DbException("connection lost during transaction", m_errno, sql);
                // try reconnecting
                hlog(HLOG_WARN, "MySQL connection lost, reconnecting...");
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
    while (ret && tries_remaining > 0);

    if (m_db == NULL)
    {
        m_error.assign("NULL database handle after query in DbMyConnection:execute()");
        return false;
    }

    if (ret) m_error = mysql_error(m_db);
    return !(ret);
}


DbResult DbMyConnection::store(const FString& sql)
{
    DbMyResult result;
    MYSQL_RES *result_ptr = NULL;
    int tries = 3;
    while (result_ptr == NULL && tries--)
    {
        if (!execute(sql)) return result;
        if ((result_ptr = mysql_store_result(m_db))!=NULL)
        {
            result = result_ptr;
        }
        else
        {
            hlog(HLOG_ERR, "DbMyConnection::store(): NULL result after successful query; errno=%u error=%s",
                 mysql_errno(m_db), mysql_error(m_db));
            // throttle back a bit
            usleep(100000); // sleep 0.1 second
        }
    }
    return result;
}


DbResult DbMyConnection::use(const FString& sql)
{
    DbMyResult result;
    if (!execute(sql)) return result;
    result = mysql_use_result(m_db);
    return result;
}


bool DbMyConnection::isTemporaryError() const
{
    return (m_errno == ER_GET_TEMPORARY_ERRMSG ||
            m_errno == ER_TRANS_CACHE_FULL ||
            m_errno == ER_LOCK_DEADLOCK ||
            m_errno == ER_LOCK_WAIT_TIMEOUT ||
            m_errno == ER_TABLE_DEF_CHANGED);
}


FString DbMyConnection::escape(const char *str)
{
    FString ret, sql(str);
    char *foo = new char[2 + 2 * sql.length()];
    mysql_escape_string(foo, sql, sql.length());
    ret = foo;
    delete [] foo;
    return ret;
}

#endif
#endif
