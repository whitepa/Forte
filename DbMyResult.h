#ifndef __DbMyResult__h
#define __DbMyResult__h

#ifndef FORTE_NO_DB
#ifdef FORTE_WITH_MYSQL

#include "DbResult.h"
#include <mysql.h>
#include <errmsg.h>
#include <mysqld_error.h>

namespace Forte
{
    class DbMyResult : public DbResult
    {
    protected:
        class MyData : public Data
        {
        public:
            // ctor/dtor
            MyData(MYSQL_RES *res);
            virtual ~MyData();

            // abstract interface
            virtual bool IsOkay() const;
            virtual bool FetchRow(DbResultRow& row /*OUT*/);
            virtual void UnFetchRow() {
                throw Exception("DbMyResult::seek not implemented");
            }
            virtual size_t GetNumColumns();
            virtual FString GetColumnName(size_t i);
            virtual size_t GetFieldLength(size_t i);
            virtual size_t GetNumRows();
            virtual bool Seek(size_t offset);

        protected:
            friend class DbMyResult;
            MYSQL_RES *mResult;
            MYSQL_ROW mRow;
            size_t mNumCols;
        };

    public:
        DbMyResult() : DbResult() { }
        DbMyResult(const DbResult& other) : DbResult(other) { }
        DbMyResult(Data *data) : DbResult(data) { }
        DbMyResult(MYSQL_RES *result);
        virtual ~DbMyResult() { }

    public:
        operator MYSQL_RES*();
    };;
};
#endif
#endif
#endif
