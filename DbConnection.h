#ifndef __DbConnection_h
#define __DbConnection_h

#ifndef FORTE_NO_DB

#include "FString.h"
#include "DbResult.h"
#include "DbException.h"
#include "AutoMutex.h"
#include <iostream>
#include <fstream>
#include "FTrace.h"

using namespace std;

namespace Forte
{
    class DbSqlStatement;

    EXCEPTION_SUBCLASS(Exception, EDbConnection);
    EXCEPTION_SUBCLASS(EDbConnection, EDbConnectionNotImplemented);
    EXCEPTION_SUBCLASS(EDbConnection, EDbConnectionConnectFailed);
    EXCEPTION_SUBCLASS(EDbConnection, EDbConnectionPendingFailed);
    EXCEPTION_SUBCLASS(EDbConnection, EDbConnectionIoError);
    EXCEPTION_SUBCLASS(EDbConnection, EDbConnectionReadOnly);

    class DbConnection : public Object
    {
    public:
        DbConnection();
        virtual ~DbConnection();

        // initialization
        virtual bool Init(const FString& db,
                          const FString& user,
                          const FString& pass,
                          const FString& host = "localhost",
                          const FString& socket = "",
                          unsigned int retries = 20);

        // connection management
        virtual bool Connect() = 0;
        virtual bool Close() = 0;

        // queries
        virtual bool Execute(const FString& sql) = 0;
        virtual DbResult Store(const FString& sql) = 0;
        virtual DbResult Use(const FString& sql) = 0;

        virtual bool Execute2(const FString& sql);
        virtual DbResult Store2(const FString& sql);
        virtual DbResult Use2(const FString& sql);

        // transactions
        virtual void AutoCommit(bool enabled);
        virtual void Begin();
        virtual void Commit();
        virtual void Rollback();
        virtual bool HasPendingQueries() const;

        // misc.
        virtual uint64_t InsertID() = 0;
        virtual uint64_t AffectedRows() = 0;
        virtual FString Escape(const char *str) = 0;

        // logging
        static void DebugFile(const char *filename);
        void LogSql(const FString &sql, const struct timeval &executionTime);

        // error information
        virtual FString GetError() const; // last database error
        virtual unsigned int GetErrno() const; // last database error code
        virtual unsigned int GetTries() const; // number of tries it took to execute the last query
        virtual bool IsTemporaryError() const = 0;

        // retry configuration
        virtual unsigned int GetRetries() const;
        virtual unsigned int GetQueryRetryDelay() const;

        virtual void BackupDatabase(const FString &targetPath);

        virtual bool Execute(const DbSqlStatement& statement);
        virtual DbResult Use(const DbSqlStatement& statement);
        virtual DbResult Store(const DbSqlStatement& statement);

        virtual const std::string& GetDbName() const;
        virtual const FString& GetCurrentQuery() const { return mCurrentQuery; }

        // db information
        FString mDBType;      // mysql, postgresql, etc.
        FString mDBName;
        FString mUser;
        FString mPassword;
        FString mHost;
        FString mSocket;
        FString mCurrentQuery;

     protected:
        FString mError;        // last database error
        unsigned int mErrno;   // last database error code
        unsigned int mTries;   // number of tries it took to execute the last query

        unsigned int mRetries;     // number of times to retry failed queries / connections
        unsigned int mQueryRetryDelay; // delay between retries in milliseconds

        bool mDidInit;
        bool mReconnect;
        bool mQueriesPending;
        bool mAutoCommit;
        bool mInTransaction;

        static ofstream sDebugOutputFile;
        static Mutex sDebugOutputMutex;
        static bool sDebugSql;
    };
};
#endif
#endif
