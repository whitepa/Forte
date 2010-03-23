#ifndef __DbMyResult__h
#define __DbMyResult__h

#ifndef FORTE_NO_DB
#ifndef FORTE_NO_MYSQL

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
            virtual bool isOkay() const;
            virtual bool fetchRow(DbResultRow& row /*OUT*/);
            virtual size_t getNumColumns();
            virtual FString getColumnName(size_t i);
            virtual size_t getFieldLength(size_t i);
            virtual size_t getNumRows();
            virtual bool seek(size_t offset);

        protected:
            friend class DbMyResult;
            MYSQL_RES *m_result;
            MYSQL_ROW m_row;
            size_t m_num_cols;
        };

    public:
        DbMyResult() : DbResult() { }
        DbMyResult(const DbResult& other) : DbResult(other) { }
        DbMyResult(Data *data) : DbResult(data) { }
        DbMyResult(MYSQL_RES *result);
        virtual ~DbMyResult() { }

    public:
        operator MYSQL_RES*();
    };
};
#endif
#endif
#endif
