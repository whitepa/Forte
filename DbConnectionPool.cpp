#ifndef FORTE_NO_DB

#include "DbConnectionPool.h"
#include "DbConnection.h"
#include "DbLiteConnection.h"
#include "DbLiteConnectionFactory.h"
#include "DbMirroredConnection.h"
#include "DbMirroredConnectionFactory.h"

#ifdef FORTE_WITH_MYSQL
#include "DbMyConnection.h"
#endif

#ifdef FORTE_WITH_PGSQL
#include "DbPgConnection.h"
#endif

#include "LogManager.h"
#include <boost/cast.hpp>
#include <boost/scoped_ptr.hpp>
#include "FTrace.h"

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
#ifdef FORTE_WITH_MYSQL
    else if (!mDbType.CompareNoCase("mysql"))
    {
        typedef DbConnectionFactoryBase<DbMyConnection> DbMyConnectionFactory;
        mDbConnectionFactory.reset(new DbMyConnectionFactory());
    }
#endif
#ifdef FORTE_WITH_PGSQL
    else if (!mDbType.CompareNoCase("postgresql"))
    {
        typedef DbConnectionFactoryBase<DbPgConnection> DbPgConnectionFactory;
        mDbConnectionFactory.reset(new DbPgConnectionFactory());
    }
#endif
#ifdef FORTE_WITH_SQLITE
    else if (!mDbType.CompareNoCase("sqlite"))
    {
        mDbConnectionFactory = shared_ptr<DbConnectionFactory>(new DbLiteConnectionFactory());
    }
    else if (!mDbType.CompareNoCase("sqlite_scribe"))
    {
        mDbConnectionFactory = shared_ptr<DbConnectionFactory>(
            new DbLiteConnectionFactory("scribeVFS"));
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
    FTRACE;

    DbConnection * pDb = NULL;
    bool initNeeded = false;
    {
        AutoUnlockMutex lock(mPoolMutex);

        hlog(HLOG_DEBUG2, "[%s] Free connections=%zu, Used connections=%zu",
             mDbName.c_str(), mFreeConnections.size(), mUsedConnections.size());
        if (mFreeConnections.size() == 0)
        {
            hlog(HLOG_DEBUG2, "[%s] Creating new connection", mDbName.c_str());
            pDb = mDbConnectionFactory->create();
            initNeeded = true;
        } else {
            hlog(HLOG_DEBUG2, "[%s] Free connections=%zu, Used connections=%zu",
                 mDbName.c_str(), mFreeConnections.size(), mUsedConnections.size());
            pDb = mFreeConnections.back();
            mUsedConnections.insert(mUsedConnections.end(), pDb);
            mFreeConnections.pop_back();
            hlog(HLOG_DEBUG2, "[%s] Free connections=%zu, Used connections=%zu",
                 mDbName.c_str(), mFreeConnections.size(), mUsedConnections.size());
        }
    }

    if (initNeeded && pDb)
    {
        auto_ptr<DbConnection> pNewDb(pDb);
        if (mDbSock.empty())
        {
            if (!pDb->Init(mDbName, mDbUser, mDbPassword, mDbHost)) {
                FString err;
                err.Format("[%s] Could not initialize database connection: %s",
                           mDbName.c_str(), pNewDb->GetError().c_str());
                throw EDbConnectionPool(err.c_str());
            }
        }
        else
        {
            if (!pNewDb->Init(mDbName, mDbUser, mDbPassword, mDbHost, mDbSock)) {
                FString err;
                err.Format("[%s] Could not initialize socket database connection: %s",
                           mDbName.c_str(), pNewDb->GetError().c_str());
                throw EDbConnectionPool(err.c_str());
            }
        }

        {
            // add this new initialized connection to used connections set
            AutoUnlockMutex lock(mPoolMutex);
            mUsedConnections.insert(mUsedConnections.end(), pNewDb.get());
            hlog(HLOG_DEBUG2, "[%s] Free connections=%zu, Used connections=%zu",
                 mDbName.c_str(), mFreeConnections.size(), mUsedConnections.size());
            // release the auto pointer at this point, we are done
            pDb = pNewDb.release();
        }
    }

    if (!pDb)
    {
        // failed to create pDb
        FString err;
        err.Format("[%s] Failed to create database connection",
                   mDbName.c_str());
        throw EDbConnectionPool(err.c_str());
    }

    hlog(HLOG_DEBUG2, "Returning connection [%p]", pDb);
    return *pDb;
}

void DbConnectionPool::ReleaseDbConnection(DbConnection& db)
{
    FTRACE;

    // make sure the connection has no pending queries
    if (db.HasPendingQueries())
    {
        hlog(HLOG_ERR, "[%s] ReleaseDbConnection(): "
             "connection has uncommitted transaction!", db.GetDbName().c_str());
        try
        {
            db.Rollback();
        }
        catch (EDbConnection& e)
        {
            hlog(HLOG_ERR, "[%s] Failed to rollback: %s.  Continuing to release." ,
                 db.GetDbName().c_str(), e.what());
        }
        catch (...)
        {
            hlog(HLOG_ERR, "[%s] Failed to rollback.  Continuing to release.",
                 db.GetDbName().c_str());
        }
    }

    DbConnection * pOldConnection = NULL;
    bool freeConnection = false;
    {
        AutoUnlockMutex lock(mPoolMutex);

        // place this connection in the free pool, and remove it from the in-use pool
        set<DbConnection*>::iterator iConnection = mUsedConnections.find(&db);

        if(iConnection != mUsedConnections.end())
        {
            hlog(HLOG_DEBUG2,
                 "[%s] BEFORE:Free connections=%zu, Used connections=%zu",
                 mDbName.c_str(), mFreeConnections.size(), mUsedConnections.size());
            mFreeConnections.push_back(*iConnection);
            mUsedConnections.erase(iConnection);
            if (mFreeConnections.size() > mPoolSize)
            {
                // have too many connections in the pool, delete one
                hlog(HLOG_DEBUG2, "[%s] Deleting connection %p (%zu > %u)",
                     mDbName.c_str(), pOldConnection,
                     mFreeConnections.size(), mPoolSize);
                pOldConnection = mFreeConnections.front();
                mFreeConnections.pop_front();
                freeConnection = true;
            }
            hlog(HLOG_DEBUG2,
                 "[%s] AFTER:Free connections=%zu, Used connections=%zu",
                 mDbName.c_str(), mFreeConnections.size(), mUsedConnections.size());
        }
        else
        {
            throw EDbConnectionPool("Connection not found");
        }
    }

    if (freeConnection && pOldConnection)
    {
        // need to free up connection
        delete pOldConnection;
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

void DbConnectionPool::OutputUsedConnectionStatus()
{
    FTRACE;
    AutoUnlockMutex lock(mPoolMutex);

    if (mUsedConnections.size() == 0) return;

    FString stuff("Used Connections:\n");

    foreach (const DbConnection *pConn, mUsedConnections)
    {
        FString line(FStringFC(),
                     "%p: %s\n", pConn, pConn->GetCurrentQuery().c_str());
        stuff.append(line);
    }
    hlog(HLOG_DEBUG, "%s", stuff.c_str());
}

#endif
