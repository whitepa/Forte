#ifndef FORTE_NO_DB
#ifndef FORTE_NO_POSTGRESQL

// DbPgResult.cpp
#include "DbPgResult.h"

// CDbPgResult class
CDbPgResult::CDbPgResult(PGresult *result)
:
    CDbResult()
{
    PgData *data = new PgData(result);
    *this = data;
}


CDbPgResult::operator PGresult*()
{
    PgData *data = dynamic_cast<PgData*>(m_data);
    if (data == NULL) return NULL;
    return data->m_result;
}


uint64_t CDbPgResult::InsertID()
{
    PgData *data = dynamic_cast<PgData*>(m_data);
    if (data == NULL) return 0;
    return data->InsertID();
}


// CDbPgResult::PgData class
CDbPgResult::PgData::PgData(PGresult *res)
:
    Data()
{
    m_result = res;
    m_num_rows = (size_t)PQntuples(m_result);
    m_num_cols = (size_t)PQnfields(m_result);
    m_current_row = 0;
}


CDbPgResult::PgData::~PgData()
{
    if (m_result != NULL) PQclear(m_result);
}


bool CDbPgResult::PgData::isOkay() const
{
    bool ret = false;

    if (m_result != NULL)
    {
        ExecStatusType code = PQresultStatus(m_result);

        switch (code)
        {
        case PGRES_EMPTY_QUERY:
        case PGRES_COMMAND_OK:
        case PGRES_TUPLES_OK:
        case PGRES_NONFATAL_ERROR:
            ret = true;
            break;
        default:
            break;
        }
    }
    
    return ret;
}


bool CDbPgResult::PgData::fetchRow(CDbResultRow& row /*OUT*/)
{
    size_t i;
    char *val;

    row.clear();
    if (m_current_row >= m_num_rows) return false;
    row.reserve(m_num_cols);

    for (i=0; i<m_num_cols; i++)
    {
        val = PQgetvalue(m_result, m_current_row, i);
        if (val[0] == 0 && PQgetisnull(m_result, m_current_row, i)) val = NULL;
        row.push_back(val);
    }

    m_current_row++;
    return true;
}


size_t CDbPgResult::PgData::getNumColumns()
{
    return m_num_cols;
}


FString CDbPgResult::PgData::getColumnName(size_t i)
{
    return PQfname(m_result, (int)i);
}


size_t CDbPgResult::PgData::getFieldLength(size_t i)
{
    return (size_t)PQgetlength(m_result, m_current_row, i);
}


uint64_t CDbPgResult::PgData::InsertID()
{
    if (m_num_rows != 1 || m_num_cols != 1) return 0;
    char *val = PQgetvalue(m_result, 0, 0);
    return strtoull(val, NULL, 10);
}

#endif
#endif
