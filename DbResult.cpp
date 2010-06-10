#ifndef FORTE_NO_DB

// DbResult.cpp
#include "Forte.h"

using namespace Forte;

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


bool DbResult::Data::FetchRow(DbRow& row /*OUT*/)
{
    DbResultRow db_row;
    if (!FetchRow(db_row)) return false;
    row.Set(db_row);
    return true;
}

#endif
