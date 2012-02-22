#ifndef __DbMyConnection_h
#define __DbMyConnection_h

#ifndef FORTE_NO_DB
#ifndef FORTE_NO_MYSQL

#include <mysql.h>
#include <errmsg.h>
#include <mysqld_error.h>
#include "FString.h"
#include "DbConnection.h"
namespace Forte
{
    class DbMyConnection : public DbConnection
    {
    public:
        typedef vector< pair<enum mysql_option, const char*> > MySQLOptionArray;

        DbMyConnection();
        DbMyConnection(const MySQLOptionArray& options);
        virtual ~DbMyConnection();

        // initialization
        virtual bool Init(const FString& db,
                          const FString& user,
                          const FString& pass,
                          const FString& host = "localhost",
                          const FString& socket = "",
                          unsigned int retries = 3);
        virtual bool Init(MYSQL *mysql);

        // connection management
        virtual bool Connect();
        virtual bool Close();

        // queries
        virtual bool Execute(const FString& sql);
        virtual DbResult Store(const FString& sql);
        virtual DbResult Use(const FString& sql);

        // error info
        virtual bool IsTemporaryError() const;

        // misc.
        virtual uint64_t InsertID() { return mysql_insert_id(mDB); }
        virtual uint64_t AffectedRows() { return mysql_affected_rows(mDB); }
        virtual FString Escape(const char *str);

    private:
        MYSQL mMySQL, *mDB;
    };
};
#endif
#endif
#endif
