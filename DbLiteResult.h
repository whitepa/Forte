#ifndef __DbLiteResult__h
#define __DbLiteResult__h

#ifndef FORTE_NO_DB
#ifndef FORTE_NO_SQLITE

#include "DbResult.h"
#include <sqlite3.h>
#include <vector>
#include <list>

namespace Forte
{
    class DbLiteConnection;

    class DbLiteResult : public DbResult
    {
    protected:
        class LiteData : public Data
        {
        public:
            // ctor/dtor
            LiteData();
            virtual ~LiteData();

            // interface to load data
            int Load(sqlite3_stmt *stmt);

            // abstract interface
            virtual bool isOkay() const;
            virtual bool fetchRow(DbResultRow& row /*OUT*/);
            virtual size_t getNumColumns();
            virtual FString getColumnName(size_t i);
            virtual size_t getFieldLength(size_t i);
            virtual size_t getNumRows();
            virtual bool seek(size_t offset);

        protected:
            friend class DbLiteResult;
            typedef std::pair<FString, bool> Val;
            typedef std::vector<Val> Row;
            typedef std::list<Row> RowList;
            typedef std::vector<FString> NameVector;
            RowList::iterator mCurrentRow, mNextRow;
            NameVector mColNames;
            RowList mRes;
            bool mOkay;
        };

    public:
        DbLiteResult() : DbResult() { }
        DbLiteResult(const DbResult& other) : DbResult(other) { }
        DbLiteResult(Data *data) : DbResult(data) { }
        virtual ~DbLiteResult() { }
        int Load(sqlite3_stmt *stmt);
    };
};
#endif
#endif
#endif
