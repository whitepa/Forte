#ifndef __DbPgConnection_h
#define __DbPgConnection_h

#ifndef FORTE_NO_DB
#ifdef FORTE_WITH_PGSQL

#include <libpq-fe.h>
#include "FString.h"
#include "DbConnection.h"
#include "DbException.h"
#include "DbPgResult.h"

namespace Forte
{
    class DbPgConnection : public DbConnection
    {
    public:
        DbPgConnection();
        virtual ~DbPgConnection();

        // initialization
        virtual bool Init(PGconn *db);

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
        virtual uint64_t InsertID() { return mLastRes.InsertID(); }
        virtual uint64_t AffectedRows() { return strtoull(PQcmdTuples(mLastRes), NULL, 0); }
        virtual FString Escape(const char *str);

    private:
        DbResult Query(const FString& sql);
        DbPgResult mLastRes;
        PGconn *mDB;
    };
};
#endif
#endif
#endif
