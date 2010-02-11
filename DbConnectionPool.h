#ifndef __DbConnectionPool_h
#define __DbConnectionPool_h

#ifndef FORTE_NO_DB

#include "AutoMutex.h"
#include "DbConnection.h"
#include "ServiceConfig.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(CForteException, CForteDbConnectionPoolException);

// A pool of database connections.
    class CDbConnectionPool {

    public:
        static CDbConnectionPool& GetInstance();
        static CDbConnectionPool& GetInstance(CServiceConfig &configObj);

        CDbConnectionPool();
        CDbConnectionPool(CServiceConfig &configObj);
        virtual ~CDbConnectionPool();

        CDbConnection& GetDbConnection();
        void ReleaseDbConnection(CDbConnection& connection);

    private:
        static auto_ptr<CDbConnectionPool> spInstance;
        static CMutex sSingletonMutex;

        FString mDbType;
        FString mDbName;
        FString mDbUser;
        FString mDbPassword;
        FString mDbHost;
        FString mDbSock;

        unsigned int mPoolSize;
        CMutex mPoolMutex;
        list<CDbConnection*> mFreeConnections;
        set<CDbConnection*> mUsedConnections;
    };
};
#endif
#endif
