#ifndef FORTE_NO_DB
#ifndef FORTE_NO_MYSQL

#include "DbMyConnection.h"
#include "DbMyResult.h"
#include "DbException.h"
#include "LogManager.h"
#include "Util.h"
#include "FTrace.h"

using namespace Forte;

DbMyConnection::DbMyConnection()
:
    DbConnection()
{
    mysql_init(&mMySQL);
    mDBType = "mysql";
    mDB = NULL;
}


DbMyConnection::DbMyConnection(const MySQLOptionArray& options)
:
    DbConnection()
{
    unsigned i, n = options.size();

    // basic init
    mysql_init(&mMySQL);
    mDBType = "mysql";
    mDB = NULL;

    // plus des options
    for (i=0; i<n; i++)
    {
        mysql_options(&mMySQL, options[i].first, options[i].second);
    }
}


DbMyConnection::~DbMyConnection()
{
    Close();
}


bool DbMyConnection::Init(MYSQL *mysql)
{
    mDB = mysql;
    mDidInit = true;
    mReconnect = false;
    return true;
}

bool DbMyConnection::Init(const FString& db,
                          const FString& user,
                          const FString& pass,
                          const FString& host,
                          const FString& socket,
                          unsigned int retries)
{
    return DbConnection::Init(db, user, pass, host, socket, retries);
}

bool DbMyConnection::Connect(void)
{
    bool ret;
    unsigned int tries = mRetries + 1;

    do
    {
        const char *sock = NULL;
        if (mSocket.length() > 0) sock = mSocket.c_str();
        mDB = mysql_real_connect(&mMySQL, mHost, mUser, mPassword,
                                  mDBName, 0, sock, 0);
        ret = (mDB != NULL);

        if (!ret)
        {
            mErrno = mysql_errno(&mMySQL);

            switch (mErrno)
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

        if (!ret && tries > 0) usleep(mQueryRetryDelay * 1000);
    }
    while (!ret && tries > 0);

    if (!ret) mError = mysql_error(&mMySQL);
    return ret;
}


bool DbMyConnection::Close(void)
{
    FTRACE;

    if (mDB != NULL) mysql_close(mDB);
    mDB = NULL;
    return true;
}


bool DbMyConnection::Execute(const FString& sql)
{
    bool ret;
    unsigned int tries_remaining = mRetries + 1;
    struct timeval tv_start, tv_end;

    // init
    mTries = 0;
    mErrno = 0;
    mError.clear();

    if (mDB == NULL && !Connect()) // try to repair the connection
    {
        mError.assign("NULL database handle in DbMyConnection::Execute()");
        return false;
    }

    if (mAutoCommit == false) mQueriesPending = true;

    do
    {
        // run query
        mTries++;
        gettimeofday(&tv_start, NULL);
        if (mDB == NULL)
        {
            mError.assign("NULL database handle before query");
            return false;
        }
        ret = mysql_real_query(mDB, sql, sql.length());
        gettimeofday(&tv_end, NULL);
        if (sDebugSql) LogSql(sql, tv_end - tv_start);

        // check for errors
        if (ret)
        {
            mErrno = mysql_errno(mDB);

            switch(mErrno)
            {
            //// soft failures, keep retrying:
            case CR_SERVER_GONE_ERROR:
            case CR_SERVER_LOST:
                if (mQueriesPending)
                    throw DbException("connection lost during transaction", mErrno, sql);
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

    if (mDB == NULL)
    {
        mError.assign("NULL database handle after query in DbMyConnection:Execute()");
        return false;
    }

    if (ret) mError = mysql_error(mDB);
    return !(ret);
}


DbResult DbMyConnection::Store(const FString& sql)
{
    DbMyResult result;
    MYSQL_RES *result_ptr = NULL;
    int tries = 3;
    while (result_ptr == NULL && tries--)
    {
        if (!Execute(sql)) return result;
        if ((result_ptr = mysql_store_result(mDB))!=NULL)
        {
            result = result_ptr;
        }
        else
        {
            hlog(HLOG_ERR, "DbMyConnection::Store(): NULL result after successful query; errno=%u error=%s",
                 mysql_errno(mDB), mysql_error(mDB));
            // throttle back a bit
            usleep(100000); // sleep 0.1 second
        }
    }
    return result;
}


DbResult DbMyConnection::Use(const FString& sql)
{
    DbMyResult result;
    if (!Execute(sql)) return result;
    result = mysql_use_result(mDB);
    return result;
}


bool DbMyConnection::IsTemporaryError() const
{
    return (mErrno == ER_GET_TEMPORARY_ERRMSG ||
            mErrno == ER_TRANS_CACHE_FULL ||
            mErrno == ER_LOCK_DEADLOCK ||
            mErrno == ER_LOCK_WAIT_TIMEOUT ||
            mErrno == ER_TABLE_DEF_CHANGED);
}


FString DbMyConnection::Escape(const char *str)
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
