#ifndef __DbLiteConnection_h
#define __DbLiteConnection_h

#ifndef FORTE_NO_DB
#ifndef FORTE_NO_SQLITE

#include <sqlite3.h>
#include "FString.h"
#include "DbConnection.h"

namespace Forte
{
    class DbLiteConnection : public DbConnection
    {
    public:
        DbLiteConnection();
        virtual ~DbLiteConnection();

        // initialization
        virtual bool Init(struct sqlite3 *db);

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
        virtual uint64_t InsertID() { return sqlite3_last_insert_rowid(m_db); }
        virtual uint64_t AffectedRows() { return sqlite3_changes(m_db); }
        virtual FString escape(const char *str);

    private:
        void set_error();
        DbResult query(const FString& sql);
        struct sqlite3 *m_db;

    public:
        int m_flags;
    };
};
#endif
#endif
#endif
