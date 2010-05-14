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
        virtual bool Execute(const FString& sql);
        virtual DbResult Store(const FString& sql);
        virtual DbResult Use(const FString& sql);

        // error info
        virtual bool IsTemporaryError() const;
    
        // misc.
        virtual uint64_t InsertID() { return sqlite3_last_insert_rowid(mDB); }
        virtual uint64_t AffectedRows() { return sqlite3_changes(mDB); }
        virtual FString Escape(const char *str);

    private:
        void setError();
        DbResult Query(const FString& sql);
        struct sqlite3 *mDB;

     public:
        int mFlags;
    };
};
#endif
#endif
#endif
