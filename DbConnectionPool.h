#ifndef __DbConnectionPool_h
#define __DbConnectionPool_h

#ifndef FORTE_NO_DB

#include <boost/shared_ptr.hpp>
#include "AutoMutex.h"
#include "DbConnection.h"
#include "DbConnectionFactory.h"
#include "ServiceConfig.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, EDbConnectionPool);
    EXCEPTION_SUBCLASS(Exception, EDbConnectionPoolOpenConnections);

// A pool of database connections.
    class DbConnectionPool : public Object {

    public:
        DbConnectionPool(ServiceConfig &configObj, const FString &root);
        DbConnectionPool(const char *dbType,
                         const char *dbName,
                         const char *dbAltName = "",
                         const char *dbUser = "",
                         const char *dbPassword = "",
                         const char *dbHost = "",
                         const char *dbSocket = "",
                         int poolSize = 0);

        virtual ~DbConnectionPool();

        virtual DbConnection& GetDbConnection(bool secondaryPreferred = false);
        virtual void ReleaseDbConnection(DbConnection& connection);
        virtual void DeleteConnections();

        const FString& GetDbName() const;
        const FString& GetBackupDbName() const;
        const FString& GetDbType() const;
        virtual void OutputUsedConnectionStatus();

    protected:
        DbConnectionPool() : mPoolSize(0) {}
        void init();

        FString mDbType;
        FString mDbName;
        FString mDbAltName;
        FString mDbUser;
        FString mDbPassword;
        FString mDbHost;
        FString mDbSock;

        unsigned int mPoolSize;
        Mutex mPoolMutex;
        list<DbConnection*> mFreeConnections;
        set<DbConnection*> mUsedConnections;
        boost::shared_ptr<DbConnectionFactory> mDbConnectionFactory;
        boost::shared_ptr<DbConnectionFactory> mDbConnectionFactoryReadOnly;
    };
    typedef boost::shared_ptr<DbConnectionPool> DbConnectionPoolPtr;
};
#endif
#endif
