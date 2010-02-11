#ifndef FORTE_NO_DB
#ifndef FORTE_NO_MYSQL

// DbMyResult.cpp
#include "DbMyResult.h"

// CDbMyResult class
CDbMyResult::CDbMyResult(MYSQL_RES *result)
:
    CDbResult()
{
    MyData *data = new MyData(result);
    *this = data;
}


CDbMyResult::operator MYSQL_RES*()
{
    MyData *data = dynamic_cast<MyData*>(m_data);
    if (data == NULL) return NULL;
    return data->m_result;
}


// CDbMyResult::MyData class
CDbMyResult::MyData::MyData(MYSQL_RES *res)
:
    Data(),
    m_result(NULL),
    m_row(NULL),
    m_num_cols(0)
{
    if (res != NULL)
    {
        m_result = res;
        m_row = NULL;
        m_num_cols = (size_t)mysql_num_fields(m_result);
    }
}


CDbMyResult::MyData::~MyData()
{
    if (m_result != NULL) mysql_free_result(m_result);
}


bool CDbMyResult::MyData::isOkay() const
{
    return (m_result != NULL);
}


bool CDbMyResult::MyData::fetchRow(CDbResultRow& row /*OUT*/)
{
    bool ret;
    size_t i;

    row.clear();

    if (m_result == NULL) return false;

    m_row = mysql_fetch_row(m_result);
    ret = (m_row != NULL);
    if (!ret) return false;
    row.reserve(m_num_cols);

    for (i=0; i<m_num_cols; i++)
    {
        row.push_back(m_row[i]);
    }
    
    return ret;
}


size_t CDbMyResult::MyData::getNumColumns()
{
    return m_num_cols;
}


FString CDbMyResult::MyData::getColumnName(size_t i)
{
    if (m_result == NULL) return "";
    MYSQL_FIELD *fields = mysql_fetch_fields(m_result);
    if (fields == NULL) return FString();
    return fields[i].name;
}


size_t CDbMyResult::MyData::getFieldLength(size_t i)
{
    if (m_result == NULL) return 0;
    unsigned long *lengths = mysql_fetch_lengths(m_result);
    if (lengths == NULL) return 0;
    return (size_t)lengths[i];
}

size_t CDbMyResult::MyData::getNumRows()
{
    if (m_result == NULL) return 0;
    return mysql_num_rows(m_result);
}

bool CDbMyResult::MyData::seek(size_t offset)
{
    if (m_result == NULL) return false;
    mysql_data_seek(m_result, offset);
    return true;
}

#endif
#endif
