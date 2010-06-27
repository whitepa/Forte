#ifndef FORTE_NO_DB
#ifndef FORTE_NO_POSTGRESQL

// DbPgConnection.cpp
#include "DbPgConnection.h"
#include "DbPgResult.h"
#include "DbException.h"
#include "AutoMutex.h"

using namespace Forte;

DbPgConnection::DbPgConnection()
:
    DbConnection()
{
    mDB = NULL;
    mDBType = "postgresql";
}


DbPgConnection::~DbPgConnection()
{
    Close();
}


bool DbPgConnection::Init(PGconn *db)
{
    mDB = db;
    mDidInit = true;
    mReconnect = false;
    return true;
}


bool DbPgConnection::Connect(void)
{
    bool ret;
    unsigned int tries = mRetries + 1;
    FString conn_info, stmp;
    vector<FString> params, parts;

    // build parameters
    if (!mSocket.empty())
    {
        stmp.Format("host='%s'", escape(mSocket).c_str());
    }
    else
    {
        parts = mHost.split(":");
        mHost = parts[0];
        stmp.Format("host='%s'", escape(mHost).c_str());

        if (parts.size() > 1)
        {
            params.push_back(stmp);
            stmp.Format("port=%u", parts[1].AsUnsignedInteger());
        }
    }

    params.push_back(stmp);

    stmp.Format("dbname='%s'", escape(mDBName).c_str());
    params.push_back(stmp);

    stmp.Format("user='%s'", escape(mUser).c_str());
    params.push_back(stmp);

    stmp.Format("password='%s'", escape(mPassword).c_str());
    params.push_back(stmp);

    // build the conn_info string
    conn_info = FString::Join(params, "\n");

    // connect to the server
    do
    {
        Close();
        mDB = PQconnectdb(conn_info);
        ret = (PQstatus(mDB) == CONNECTION_OK);

        if (!ret)
        {
            mErrno = PQstatus(mDB) + 1000;
            --tries;
        }

        if (!ret && tries > 0) usleep(mQueryRetryDelay * 1000);
    }
    while (!ret && tries > 0);

    if (!ret)
    {
        mError = PQerrorMessage(mDB);
        Close();
    }

    return ret;
}


bool DbPgConnection::Close(void)
{
    if (mDB != NULL) PQfinish(mDB);
    mDB = NULL;
    return true;
}


DbResult DbPgConnection::Query(const FString& sql)
{
    unsigned int tries_remaining = mRetries + 1;
    struct timeval tv_start, tv_end;
    DbPgResult res;
    int code;

    // init
    mTries = 0;
    mErrno = 0;
    mError.clear();

    if (mDB == NULL)
    {
        mError.assign("Connection is gone.");
        mErrno = CONNECTION_BAD + 1000;
        mLastRes = res;
        return res;
    }

    if (mAutoCommit == false) mQueriesPending = true;

    do
    {
        // run query
        mTries++;
        gettimeofday(&tv_start, NULL);
        res = PQexec(mDB, sql);
        gettimeofday(&tv_end, NULL);
        if (sDebugSql) LogSql(sql, tv_end - tv_start);
        if ((PGresult*)res == NULL) code = CONNECTION_BAD + 1000;
        else code = PQresultStatus(res);

        // check for errors
        if (!res.IsOkay() ||
            ((code != PGRES_EMPTY_QUERY) &&
             (code != PGRES_COMMAND_OK) &&
             (code != PGRES_TUPLES_OK)))
        {
            mErrno = code;
            res.Clear();

            switch (mErrno)
            {
            //// soft failures, keep retrying:
            case (CONNECTION_BAD + 1000):
            case PGRES_BAD_RESPONSE:
                if (mQueriesPending)
                    throw DbException("connection lost during transaction", mErrno, sql);
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

    mLastRes = res;

    if (mDB == NULL)
    {
        mError.assign("Connection lost; unable to reestablish.");
        return res;
    }

    if (!res) mError = PQerrorMessage(mDB);
    return res;
}


bool DbPgConnection::Execute(const FString& sql)
{
    return Query(sql);
}


DbResult DbPgConnection::Store(const FString& sql)
{
    return Query(sql);
}


DbResult DbPgConnection::use(const FString& sql)
{
    return Query(sql);
}


bool DbPgConnection::isTemporaryError() const
{
    return (mErrno == PGRES_COPY_OUT ||
            mErrno == PGRES_COPY_IN);
}


FString DbPgConnection::escape(const char *str)
{
    FString ret, sql(str);;
    char *foo = new char[2 + 2 * sql.length()];

    if (mDB != NULL)
    {
        // use correct method
        PQescapeStringConn(mDB, foo, sql, sql.length(), NULL);
    }
    else
    {
        // use deprecated function
        static Mutex mutex;
        AutoUnlockMutex lock(mutex);
        PQescapeString(foo, sql, sql.length());
    }

    ret = foo;
    delete [] foo;
    return ret;
}

#endif
#endif
