#ifndef FORTE_NO_DB

#include "Forte.h"
#include "DbConnectionPool.h"
#include "DbConnection.h"

auto_ptr<CDbConnectionPool> CDbConnectionPool::spInstance;
CMutex CDbConnectionPool::sSingletonMutex;

CDbConnectionPool& CDbConnectionPool::GetInstance() {
    if (spInstance.get() == NULL) {
        CAutoUnlockMutex lock(sSingletonMutex);
        if (spInstance.get() == NULL) {
            spInstance.reset(new CDbConnectionPool());
        }
    }
    return *spInstance;
}

CDbConnectionPool& CDbConnectionPool::GetInstance(CServiceConfig &configObj) {
    if (spInstance.get() == NULL) {
        CAutoUnlockMutex lock(sSingletonMutex);
        if (spInstance.get() == NULL) {
            spInstance.reset(new CDbConnectionPool(configObj));
        }
    }
    return *spInstance;
}


CDbConnectionPool::CDbConnectionPool() : 
    mDbType(CServerMain::GetServer().mServiceConfig.Get("db_type")),
    mDbName(CServerMain::GetServer().mServiceConfig.Get("db_name")),
    mDbUser(CServerMain::GetServer().mServiceConfig.Get("db_user")),
    mDbPassword(CServerMain::GetServer().mServiceConfig.Get("db_pass")),
    mDbHost(CServerMain::GetServer().mServiceConfig.Get("db_host")),
    mDbSock(CServerMain::GetServer().mServiceConfig.Get("db_socket")),
    mPoolSize(CServerMain::GetServer().mServiceConfig.GetInteger("db_poolsize"))
{
    if (mDbType.empty())
        throw CForteDbConnectionPoolException("'db_type' must be specified in the database configuration");
    if (CServerMain::GetServer().mServiceConfig.Get("db_poolsize").empty())
        throw CForteDbConnectionPoolException("'db_poolsize' must be specified in the database configuration");
}

CDbConnectionPool::CDbConnectionPool(CServiceConfig &configObj) :
    mDbType(configObj.Get("db_type")),
    mDbName(configObj.Get("db_name")),
    mDbUser(configObj.Get("db_user")),
    mDbPassword(configObj.Get("db_pass")),
    mDbHost(configObj.Get("db_host")),
    mDbSock(configObj.Get("db_socket")),
    mPoolSize(configObj.GetInteger("db_poolsize"))
{
    if (mDbType.empty())
        throw CForteDbConnectionPoolException("'db_type' must be specified in the database configuration");
    if (configObj.Get("db_poolsize").empty())
        throw CForteDbConnectionPoolException("'db_poolsize' must be specified in the database configuration");
}

CDbConnectionPool::~CDbConnectionPool() {
}

CDbConnection& CDbConnectionPool::GetDbConnection() {
    CAutoUnlockMutex lock(mPoolMutex);
    CDbConnection * pDb;

    if (mFreeConnections.size() == 0)
    {
        CDbConnection *new_db = NULL;

        if (false) { }
#ifndef FORTE_NO_MYSQL
        else if (!mDbType.CompareNoCase("mysql"))
        {
            new_db = static_cast<CDbConnection*>(new CDbMyConnection());
        }
#endif
#ifndef FORTE_NO_POSTGRESQL
        else if (!mDbType.CompareNoCase("postgresql"))
        {
            new_db = static_cast<CDbConnection*>(new CDbPgConnection());
        }
#endif
#ifndef FORTE_NO_SQLITE
        else if (!mDbType.CompareNoCase("sqlite"))
        {
            new_db = static_cast<CDbConnection*>(new CDbLiteConnection());
        }
#endif
        else
        {
            FString err;
            err.Format("Unrecognized database type: %s", mDbType.c_str());
            throw CForteDbConnectionPoolException(err);
        }

        auto_ptr<CDbConnection> pNewDb(new_db);
        
        if (mDbSock.empty())
        {
            if (!pNewDb->Init(mDbName, mDbUser, mDbPassword, mDbHost)) {
                FString err;
                err.Format("Could not initialize database connection: %s",
                           pNewDb->m_error.c_str());
                throw CForteDbConnectionPoolException(err.c_str());
            }
        }
        else
        {
            if (!pNewDb->Init(mDbName, mDbUser, mDbPassword, mDbHost, mDbSock)) {
                FString err;
                err.Format("Could not initialize socket database connection: %s",
                           pNewDb->m_error.c_str());
                throw CForteDbConnectionPoolException(err.c_str());
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

void CDbConnectionPool::ReleaseDbConnection(CDbConnection& db)
{
    // make sure the connection has no pending queries
    if (db.hasPendingQueries())
    {
        hlog(HLOG_ERR, "ReleaseDbConnection(): connection has uncommitted transaction!");
        db.rollback();
    }
    CAutoUnlockMutex lock(mPoolMutex);
    
    // place this connection in the free pool, and remove it from the in-use pool
    set<CDbConnection*>::iterator iConnection = mUsedConnections.find(&db);
    mFreeConnections.push_back(*iConnection);
    mUsedConnections.erase(iConnection);

    if (mFreeConnections.size() > mPoolSize)
    {
        // have too many connections in the pool, delete one
        CDbConnection * pOldConnection = mFreeConnections.front();
        mFreeConnections.pop_front();
        delete pOldConnection;
    }
}

#endif
