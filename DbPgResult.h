#ifndef __DbPgResult__h
#define __DbPgResult__h

#ifndef FORTE_NO_DB
#ifndef FORTE_NO_POSTGRESQL

#include "DbResult.h"
#include "Exception.h"
#include <libpq-fe.h>

class CDbPgResult : public CDbResult
{
protected:
    class PgData : public Data
    {
    public:
        // ctor/dtor
        PgData(PGresult *res);
        virtual ~PgData();

        // abstract interface
        virtual bool isOkay() const;
        virtual bool fetchRow(CDbResultRow& row /*OUT*/);
        virtual size_t getNumColumns();
        virtual FString getColumnName(size_t i);
        virtual size_t getFieldLength(size_t i);
        virtual size_t getNumRows() { 
            throw CException("DbPgResult::getNumRows not implemented");
        }
        virtual bool seek(size_t offset) {
            throw CException("DbPgResult::seek not implemented");
        }
        // additions for PostgreSQL only
        virtual uint64_t InsertID();

    protected:
        friend class CDbPgResult;
        PGresult *m_result;
        size_t m_num_rows;
        size_t m_num_cols;
        size_t m_current_row;
    };

public:
    CDbPgResult() : CDbResult() { }
    CDbPgResult(const CDbResult& other) : CDbResult(other) { }
    CDbPgResult(Data *data) : CDbResult(data) { }
    CDbPgResult(PGresult *result);
    virtual ~CDbPgResult() { }
    virtual uint64_t InsertID();

public:
    operator PGresult*();
};

#endif
#endif
#endif
