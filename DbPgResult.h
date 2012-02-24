#ifndef __DbPgResult__h
#define __DbPgResult__h

#ifndef FORTE_NO_DB
#ifdef FORTE_WITH_PGSQL

#include "DbResult.h"
#include "Exception.h"
#include <libpq-fe.h>

namespace Forte
{
    class DbPgResult : public DbResult
    {
    protected:
        class PgData : public Data
        {
        public:
            // ctor/dtor
            PgData(PGresult *res);
            virtual ~PgData();

            // abstract interface
            virtual bool IsOkay() const;
            virtual bool FetchRow(DbResultRow& row /*OUT*/);
            virtual void UnFetchRow();
            virtual size_t GetNumColumns();
            virtual FString GetColumnName(size_t i);
            virtual size_t GetFieldLength(size_t i);
            virtual size_t GetNumRows() {
                throw Exception("DbPgResult::getNumRows not implemented");
            }
            virtual bool Seek(size_t offset) {
                throw Exception("DbPgResult::seek not implemented");
            }
            // additions for PostgreSQL only
            virtual uint64_t InsertID();

         protected:
            friend class DbPgResult;
            PGresult *mResult;
            size_t mNumRows;
            size_t mNumCols;
            size_t mCurrentRow;
        };

    public:
        DbPgResult() : DbResult() { }
        DbPgResult(const DbResult& other) : DbResult(other) { }
        DbPgResult(Data *data) : DbResult(data) { }
        DbPgResult(PGresult *result);
        virtual ~DbPgResult() { }
        virtual uint64_t InsertID();

    public:
        operator PGresult*();
    };
};
#endif
#endif
#endif
