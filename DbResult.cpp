#ifndef FORTE_NO_DB

// DbResult.cpp
#include "Forte.h"

DbResult::DbResult()
{
    m_data = NULL;
}


void DbResult::Clear()
{
    if (m_data) m_data->Release();
    m_data = NULL;
}


DbResult& DbResult::operator =(const DbResult& other)
{
    return (*this = other.m_data);
}


DbResult& DbResult::operator =(DbResult::Data *data)
{
    // ordered so that "*this = this->m_data;" will still work
    if (data) data->AddRef();
    if (m_data) m_data->Release();
    m_data = data;
    return *this;
}


bool DbResult::Data::fetchRow(DbRow& row /*OUT*/)
{
    DbResultRow db_row;
    if (!fetchRow(db_row)) return false;
    row.Set(db_row);
    return true;
}

#endif
