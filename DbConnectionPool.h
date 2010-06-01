#ifndef __DbConnectionPool_h
#define __DbConnectionPool_h

#ifndef FORTE_NO_DB

#include "AutoMutex.h"
#include "DbConnection.h"
#include "ServiceConfig.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, EDbConnectionPool);
    EXCEPTION_SUBCLASS(Exception, EDbConnectionPoolOpenConnections);

// A pool of database connections.
    class DbConnectionPool : public Object {

    public:

        DbConnectionPool();
        DbConnectionPool(ServiceConfig &configObj);
        DbConnectionPool(const char *dbType,
                         const char *dbName,
                         const char *dbUser = "",
                         const char *dbPassword = "",
                         const char *dbHost = "",
                         const char *dbSocket = "",
                         int poolSize = 0);

        virtual ~DbConnectionPool();

        DbConnection& GetDbConnection();
        void ReleaseDbConnection(DbConnection& connection);
        void DeleteConnections();

    private:
        static auto_ptr<DbConnectionPool> spInstance;
        static Mutex sSingletonMutex;

        FString mDbType;
        FString mDbName;
        FString mDbUser;
        FString mDbPassword;
        FString mDbHost;
        FString mDbSock;

        unsigned int mPoolSize;
        Mutex mPoolMutex;
        list<DbConnection*> mFreeConnections;
        set<DbConnection*> mUsedConnections;
    };
};
#endif
#endif
