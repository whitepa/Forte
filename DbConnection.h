#ifndef __DbConnection_h
#define __DbConnection_h

#ifndef FORTE_NO_DB

#include "FString.h"
#include "DbResult.h"
#include "DbException.h"
namespace Forte
{
    class DbConnection : public Object
    {
    public:
        DbConnection();
        virtual ~DbConnection();

        // initialization
        virtual bool Init(const FString& db, const FString& user, const FString& pass,
                          const FString& host = "localhost", const FString& socket = "",
                          unsigned int retries = 3);

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
        inline bool HasPendingQueries() { return mQueriesPending; }
    
        // misc.
        virtual uint64_t InsertID() = 0;
        virtual uint64_t AffectedRows() = 0;
        virtual FString Escape(const char *str) = 0;

        // logging
        static void DebugFile(const char *filename);
        void LogSql(const FString &sql, const struct timeval &executionTime);

        bool mLogQueries;     // flag to turn (hlog) logging on/off on
        // the fly (should be used when temporary
        // logging is needed, auto connection will
        // turn this off when released)
        inline void LogQueries(bool log) { mLogQueries = log; }

        // error information
        FString mError;        // last database error
        unsigned int mErrno;   // last database error code
        unsigned int mTries;   // number of tries it took to execute the last query
        virtual bool IsTemporaryError() const = 0;
    
        // retry configuration
        unsigned int mRetries;     // number of times to retry failed queries / connections
        unsigned int mQueryRetryDelay; // delay between retries in milliseconds

        // db information
        FString mDBType;      // mysql, postgresql, etc.
        FString mDBName;
        FString mUser;
        FString mPassword;
        FString mHost;
        FString mSocket;

     protected:
        bool mDidInit;
        bool mReconnect;
        bool mQueriesPending;
        bool mAutoCommit;

        static ofstream sDebugOutputFile;
        static Mutex sDebugOutputMutex;
        static bool sDebugSql;
    };
};
#endif
#endif
