#ifndef __DbLiteResult__h
#define __DbLiteResult__h

#ifndef FORTE_NO_DB
#ifndef FORTE_NO_SQLITE

#include "DbResult.h"
#include <sqlite3.h>
#include <vector>
#include <list>

class CDbLiteConnection;

class CDbLiteResult : public CDbResult
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
        virtual bool fetchRow(CDbResultRow& row /*OUT*/);
        virtual size_t getNumColumns();
        virtual FString getColumnName(size_t i);
        virtual size_t getFieldLength(size_t i);
        virtual size_t getNumRows();
        virtual bool seek(size_t offset);

    protected:
        friend class CDbLiteResult;
        typedef std::pair<FString, bool> Val;
        typedef std::vector<Val> Row;
        typedef std::list<Row> RowList;
        typedef std::vector<FString> NameVector;
        RowList::iterator m_current_row, m_next_row;
        NameVector m_col_names;
        RowList m_res;
        bool m_okay;
    };

public:
    CDbLiteResult() : CDbResult() { }
    CDbLiteResult(const CDbResult& other) : CDbResult(other) { }
    CDbLiteResult(Data *data) : CDbResult(data) { }
    virtual ~CDbLiteResult() { }
    int Load(sqlite3_stmt *stmt);
};

#endif
#endif
#endif
