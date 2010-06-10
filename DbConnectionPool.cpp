#ifndef FORTE_NO_DB

#include "Forte.h"
#include "DbConnectionPool.h"
#include "DbConnection.h"

using namespace Forte;

DbConnectionPool::DbConnectionPool(const char *dbType,
                                   const char *dbName,
                                   const char *dbUser,
                                   const char *dbPassword,
                                   const char *dbHost,
                                   const char *dbSocket,
                                   int poolSize) :
    mDbType(dbType), mDbName(dbName), mDbUser(dbUser), mDbPassword(dbPassword),
    mDbHost(dbHost), mDbSock(dbSocket), mPoolSize(poolSize)
{
    if (mDbType.empty())
        throw EDbConnectionPool("'db_type' must be specified in the database configuration");
}

DbConnectionPool::~DbConnectionPool() {
}

void DbConnectionPool::DeleteConnections()
{
    AutoUnlockMutex lock(mPoolMutex);
    if (mUsedConnections.size() > 0)
    {
        throw EDbConnectionPoolOpenConnections("Connections are still open");
    }

    DbConnection* pDb;
    while (mFreeConnections.size() > 0)
    {
        pDb = mFreeConnections.back();
        mFreeConnections.pop_back();
        delete pDb;
    }

    mFreeConnections.erase(mFreeConnections.begin(), mFreeConnections.end());
}

DbConnection& DbConnectionPool::GetDbConnection() {
    AutoUnlockMutex lock(mPoolMutex);
    DbConnection * pDb;

    if (mFreeConnections.size() == 0)
    {
        DbConnection *new_db = NULL;

        if (false) { }
#ifndef FORTE_NO_MYSQL
        else if (!mDbType.CompareNoCase("mysql"))
        {
            new_db = static_cast<DbConnection*>(new DbMyConnection());
        }
#endif
#ifndef FORTE_NO_POSTGRESQL
        else if (!mDbType.CompareNoCase("postgresql"))
        {
            new_db = static_cast<DbConnection*>(new DbPgConnection());
        }
#endif
#ifndef FORTE_NO_SQLITE
        else if (!mDbType.CompareNoCase("sqlite"))
        {
            new_db = static_cast<DbConnection*>(new DbLiteConnection());
        }
#endif
        else
        {
            FString err;
            err.Format("Unrecognized database type: %s", mDbType.c_str());
            throw EDbConnectionPool(err);
        }

        auto_ptr<DbConnection> pNewDb(new_db);
        
        if (mDbSock.empty())
        {
            if (!pNewDb->Init(mDbName, mDbUser, mDbPassword, mDbHost)) {
                FString err;
                err.Format("Could not initialize database connection: %s",
                           pNewDb->mError.c_str());
                throw EDbConnectionPool(err.c_str());
            }
        }
        else
        {
            if (!pNewDb->Init(mDbName, mDbUser, mDbPassword, mDbHost, mDbSock)) {
                FString err;
                err.Format("Could not initialize socket database connection: %s",
                           pNewDb->mError.c_str());
                throw EDbConnectionPool(err.c_str());
            }
        }
        mUsedConnections.insert(mUsedConnections.end(), pNewDb.get());
        pDb = pNewDb.release();
    } else {
        pDb = mFreeConnections.back();
        mUsedConnections.insert(mUsedConnections.end(), pDb);
        mFreeConnections.pop_back();
    }
    return *pDb;
}

void DbConnectionPool::ReleaseDbConnection(DbConnection& db)
{
    // make sure the connection has no pending queries
    if (db.HasPendingQueries())
    {
        hlog(HLOG_ERR, "ReleaseDbConnection(): connection has uncommitted transaction!");
        db.Rollback();
    }
    AutoUnlockMutex lock(mPoolMutex);
    
    // place this connection in the free pool, and remove it from the in-use pool
    set<DbConnection*>::iterator iConnection = mUsedConnections.find(&db);
    mFreeConnections.push_back(*iConnection);
    mUsedConnections.erase(iConnection);

    if (mFreeConnections.size() > mPoolSize)
    {
        // have too many connections in the pool, delete one
        DbConnection * pOldConnection = mFreeConnections.front();
        mFreeConnections.pop_front();
        delete pOldConnection;
    }
}

#endif
