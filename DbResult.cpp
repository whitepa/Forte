#ifndef FORTE_NO_DB

// DbResult.cpp
#include "Forte.h"

CDbResult::CDbResult()
{
    m_data = NULL;
}


void CDbResult::Clear()
{
    if (m_data) m_data->Release();
    m_data = NULL;
}


CDbResult& CDbResult::operator =(const CDbResult& other)
{
    return (*this = other.m_data);
}


CDbResult& CDbResult::operator =(CDbResult::Data *data)
{
    // ordered so that "*this = this->m_data;" will still work
    if (data) data->AddRef();
    if (m_data) m_data->Release();
    m_data = data;
    return *this;
}


bool CDbResult::Data::fetchRow(CDbRow& row /*OUT*/)
{
    CDbResultRow db_row;
    if (!fetchRow(db_row)) return false;
    row.Set(db_row);
    return true;
}

#endif
