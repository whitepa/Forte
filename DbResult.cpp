#ifndef FORTE_NO_DB

// DbResult.cpp
#include "Forte.h"

DbResult::DbResult()
{
    mData = NULL;
}


void DbResult::Clear()
{
    if (mData) mData->Release();
    mData = NULL;
}


DbResult& DbResult::operator =(const DbResult& other)
{
    return (*this = other.mData);
}


DbResult& DbResult::operator =(DbResult::Data *data)
{
    // ordered so that "*this = this->mData;" will still work
    if (data) data->AddRef();
    if (mData) mData->Release();
    mData = data;
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
