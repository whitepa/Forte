#ifndef __DbMyResult__h
#define __DbMyResult__h

#ifndef FORTE_NO_DB
#ifndef FORTE_NO_MYSQL

#include "DbResult.h"
#include <mysql.h>
#include <errmsg.h>
#include <mysqld_error.h>

class CDbMyResult : public CDbResult
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
        virtual bool fetchRow(CDbResultRow& row /*OUT*/);
        virtual size_t getNumColumns();
        virtual FString getColumnName(size_t i);
        virtual size_t getFieldLength(size_t i);
        virtual size_t getNumRows();
        virtual bool seek(size_t offset);

    protected:
        friend class CDbMyResult;
        MYSQL_RES *m_result;
        MYSQL_ROW m_row;
        size_t m_num_cols;
    };

public:
    CDbMyResult() : CDbResult() { }
    CDbMyResult(const CDbResult& other) : CDbResult(other) { }
    CDbMyResult(Data *data) : CDbResult(data) { }
    CDbMyResult(MYSQL_RES *result);
    virtual ~CDbMyResult() { }

public:
    operator MYSQL_RES*();
};

#endif
#endif
#endif
