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
        virtual bool execute(const FString& sql) = 0;
        virtual DbResult store(const FString& sql) = 0;
        virtual DbResult use(const FString& sql) = 0;

        virtual bool execute2(const FString& sql);
        virtual DbResult store2(const FString& sql);
        virtual DbResult use2(const FString& sql);

        // transactions
        virtual void autoCommit(bool enabled);
        virtual void begin();
        virtual void commit();
        virtual void rollback();
        inline bool hasPendingQueries() { return mQueriesPending; }
    
        // misc.
        virtual uint64_t InsertID() = 0;
        virtual uint64_t AffectedRows() = 0;
        virtual FString escape(const char *str) = 0;

        // logging
        static void debugFile(const char *filename);
        void logSql(const FString &sql, const struct timeval &executionTime);

        bool mLogQueries;     // flag to turn (hlog) logging on/off on
        // the fly (should be used when temporary
        // logging is needed, auto connection will
        // turn this off when released)
        inline void logQueries(bool log) { mLogQueries = log; }

        // error information
        FString mError;        // last database error
        unsigned int mErrno;   // last database error code
        unsigned int mTries;   // number of tries it took to execute the last query
        virtual bool isTemporaryError() const = 0;
    
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
