#ifndef __DbConnection_h
#define __DbConnection_h

#ifndef FORTE_NO_DB

#include "FString.h"
#include "DbResult.h"
#include "DbException.h"
namespace Forte
{
    class CDbConnection
    {
    public:
        CDbConnection();
        virtual ~CDbConnection();

        // initialization
        virtual bool Init(const FString& db, const FString& user, const FString& pass,
                          const FString& host = "localhost", const FString& socket = "",
                          unsigned int retries = 3);

        // connection management
        virtual bool Connect() = 0;
        virtual bool Close() = 0;

        // queries
        virtual bool execute(const FString& sql) = 0;
        virtual CDbResult store(const FString& sql) = 0;
        virtual CDbResult use(const FString& sql) = 0;

        virtual bool execute2(const FString& sql);
        virtual CDbResult store2(const FString& sql);
        virtual CDbResult use2(const FString& sql);

        // transactions
        virtual void autoCommit(bool enabled);
        virtual void begin();
        virtual void commit();
        virtual void rollback();
        inline bool hasPendingQueries() { return m_queries_pending; }
    
        // misc.
        virtual uint64_t InsertID() = 0;
        virtual uint64_t AffectedRows() = 0;
        virtual FString escape(const char *str) = 0;

        // logging
        static void debugFile(const char *filename);
        void logSql(const FString &sql, const struct timeval &executionTime);

        bool m_log_queries;     // flag to turn (hlog) logging on/off on
        // the fly (should be used when temporary
        // logging is needed, auto connection will
        // turn this off when released)
        inline void logQueries(bool log) { m_log_queries = log; }

        // error information
        FString m_error;        // last database error
        unsigned int m_errno;   // last database error code
        unsigned int m_tries;   // number of tries it took to execute the last query
        virtual bool isTemporaryError() const = 0;
    
        // retry configuration
        unsigned int m_retries;     // number of times to retry failed queries / connections
        unsigned int m_query_retry_delay; // delay between retries in milliseconds

        // db information
        FString m_db_type;      // mysql, postgresql, etc.
        FString m_db_name;
        FString m_user;
        FString m_password;
        FString m_host;
        FString m_socket;

    protected:
        bool m_did_init;
        bool m_reconnect;
        bool m_queries_pending;
        bool m_autocommit;

        static ofstream sDebugOutputFile;
        static CMutex sDebugOutputMutex;
        static bool sDebugSql;
    };
};
#endif
#endif
