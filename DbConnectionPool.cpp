#ifndef FORTE_NO_DB

#include "DbConnectionPool.h"
#include "DbConnection.h"
#include "DbLiteConnection.h"
#include "DbLiteConnectionFactory.h"
#include "DbMirroredConnection.h"
#include "DbMirroredConnectionFactory.h"

#ifndef FORTE_NO_MYSQL
#include "DbMyConnection.h"
#endif

#ifndef FORTE_NO_POSTGRESQL
#include "DbPgConnection.h"
#endif

#include "LogManager.h"
#include <boost/cast.hpp>
#include <boost/scoped_ptr.hpp>

using namespace Forte;
using namespace boost;

DbConnectionPool::DbConnectionPool(const char *dbType,
                                   const char *dbName,
                                   const char *dbAltName,
                                   const char *dbUser,
                                   const char *dbPassword,
                                   const char *dbHost,
                                   const char *dbSocket,
                                   int poolSize) :
    mDbType(dbType), mDbName(dbName), mDbAltName(dbAltName), mDbUser(dbUser), mDbPassword(dbPassword),
    mDbHost(dbHost), mDbSock(dbSocket), mPoolSize(poolSize)
{
    if (mDbType.empty())
        throw EDbConnectionPool("'db_type' must be specified in the database configuration");

    if (false) { }
#ifndef FORTE_NO_MYSQL
    else if (!mDbType.CompareNoCase("mysql"))
    {
        typedef DbConnectionFactoryBase<DbMyConnection> DbMyConnectionFactory;
        mDbConnectionFactory.reset(new DbMyConnectionFactory());
    }
#endif
#ifndef FORTE_NO_POSTGRESQL
    else if (!mDbType.CompareNoCase("postgresql"))
    {
        typedef DbConnectionFactoryBase<DbPgConnection> DbPgConnectionFactory;
        mDbConnectionFactory.reset(new DbPgConnectionFactory());
    }
#endif
#ifndef FORTE_NO_SQLITE
    else if (!mDbType.CompareNoCase("sqlite"))
    {
        mDbConnectionFactory = shared_ptr<DbConnectionFactory>(new DbLiteConnectionFactory());
    }
    else if (!mDbType.CompareNoCase("sqlite_mirrored"))
    {
        shared_ptr<DbConnectionFactory> primaryDbFactory(new DbLiteConnectionFactory());
        shared_ptr<DbConnectionFactory> secondaryDbFactory(new DbLiteConnectionFactory(SQLITE_OPEN_READONLY));
        mDbConnectionFactory.reset(new DbMirroredConnectionFactory(primaryDbFactory, secondaryDbFactory, dbAltName));
    }
#endif
    else
    {
        FString err;
        err.Format("Unrecognized database type: %s", mDbType.c_str());
        throw EDbConnectionPool(err);
    }
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
        auto_ptr<DbConnection> pNewDb(mDbConnectionFactory->create());
        
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

    if(iConnection != mUsedConnections.end())
    {
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
    else
    {
        throw EDbConnectionPool("Connection not found");
    }
}

const FString& DbConnectionPool::GetDbName() const
{
    return mDbName;
}

const FString& DbConnectionPool::GetBackupDbName() const
{
    return mDbAltName;
}

const FString& DbConnectionPool::GetDbType() const
{
    return mDbType;
}

#endif
