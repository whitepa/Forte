#ifndef FORTE_NO_DB

#include "Forte.h"
#include "DbConnectionPool.h"
#include "DbConnection.h"

auto_ptr<DbConnectionPool> DbConnectionPool::spInstance;
Mutex DbConnectionPool::sSingletonMutex;

DbConnectionPool& DbConnectionPool::GetInstance() {
    if (spInstance.get() == NULL) {
        AutoUnlockMutex lock(sSingletonMutex);
        if (spInstance.get() == NULL) {
            spInstance.reset(new DbConnectionPool());
        }
    }
    return *spInstance;
}

DbConnectionPool& DbConnectionPool::GetInstance(ServiceConfig &configObj) {
    if (spInstance.get() == NULL) {
        AutoUnlockMutex lock(sSingletonMutex);
        if (spInstance.get() == NULL) {
            spInstance.reset(new DbConnectionPool(configObj));
        }
    }
    return *spInstance;
}


DbConnectionPool::DbConnectionPool() : 
    mDbType(ServerMain::GetServer().mServiceConfig.Get("db_type")),
    mDbName(ServerMain::GetServer().mServiceConfig.Get("db_name")),
    mDbUser(ServerMain::GetServer().mServiceConfig.Get("db_user")),
    mDbPassword(ServerMain::GetServer().mServiceConfig.Get("db_pass")),
    mDbHost(ServerMain::GetServer().mServiceConfig.Get("db_host")),
    mDbSock(ServerMain::GetServer().mServiceConfig.Get("db_socket")),
    mPoolSize(ServerMain::GetServer().mServiceConfig.GetInteger("db_poolsize"))
{
    if (mDbType.empty())
        throw ForteDbConnectionPoolException("'db_type' must be specified in the database configuration");
    if (ServerMain::GetServer().mServiceConfig.Get("db_poolsize").empty())
        throw ForteDbConnectionPoolException("'db_poolsize' must be specified in the database configuration");
}

DbConnectionPool::DbConnectionPool(ServiceConfig &configObj) :
    mDbType(configObj.Get("db_type")),
    mDbName(configObj.Get("db_name")),
    mDbUser(configObj.Get("db_user")),
    mDbPassword(configObj.Get("db_pass")),
    mDbHost(configObj.Get("db_host")),
    mDbSock(configObj.Get("db_socket")),
    mPoolSize(configObj.GetInteger("db_poolsize"))
{
    if (mDbType.empty())
        throw ForteDbConnectionPoolException("'db_type' must be specified in the database configuration");
    if (configObj.Get("db_poolsize").empty())
        throw ForteDbConnectionPoolException("'db_poolsize' must be specified in the database configuration");
}

DbConnectionPool::~DbConnectionPool() {
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
            throw ForteDbConnectionPoolException(err);
        }

        auto_ptr<DbConnection> pNewDb(new_db);
        
        if (mDbSock.empty())
        {
            if (!pNewDb->Init(mDbName, mDbUser, mDbPassword, mDbHost)) {
                FString err;
                err.Format("Could not initialize database connection: %s",
                           pNewDb->m_error.c_str());
                throw ForteDbConnectionPoolException(err.c_str());
            }
        }
        else
        {
            if (!pNewDb->Init(mDbName, mDbUser, mDbPassword, mDbHost, mDbSock)) {
                FString err;
                err.Format("Could not initialize socket database connection: %s",
                           pNewDb->m_error.c_str());
                throw ForteDbConnectionPoolException(err.c_str());
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
    if (db.hasPendingQueries())
    {
        hlog(HLOG_ERR, "ReleaseDbConnection(): connection has uncommitted transaction!");
        db.rollback();
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
