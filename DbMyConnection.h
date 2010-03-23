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
        virtual bool Init(MYSQL *mysql);

        // connection management
        virtual bool Connect();
        virtual bool Close();

        // queries
        virtual bool execute(const FString& sql);
        virtual DbResult store(const FString& sql);
        virtual DbResult use(const FString& sql);

        // error info
        virtual bool isTemporaryError() const;
    
        // misc.
        virtual uint64_t InsertID() { return mysql_insert_id(m_db); }
        virtual uint64_t AffectedRows() { return mysql_affected_rows(m_db); }
        virtual FString escape(const char *str);

    private:
        MYSQL m_mysql, *m_db;
    };
};
#endif
#endif
#endif
