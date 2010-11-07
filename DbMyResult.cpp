#ifndef FORTE_NO_DB
#ifndef FORTE_NO_MYSQL

// DbMyResult.cpp
#include "DbMyResult.h"

using namespace Forte;

// DbMyResult class
DbMyResult::DbMyResult(MYSQL_RES *result)
:
    DbResult()
{
    MyData *data = new MyData(result);
    *this = data;
}


DbMyResult::operator MYSQL_RES*()
{
    MyData *data = dynamic_cast<MyData*>(mData);
    if (data == NULL) return NULL;
    return data->mResult;
}


// DbMyResult::MyData class
DbMyResult::MyData::MyData(MYSQL_RES *res)
:
    Data(),
    mResult(NULL),
    mRow(NULL),
    mNumCols(0)
{
    if (res != NULL)
    {
        mResult = res;
        mRow = NULL;
        mNumCols = (size_t)mysql_num_fields(mResult);
    }
}


DbMyResult::MyData::~MyData()
{
    if (mResult != NULL) mysql_free_result(mResult);
}


bool DbMyResult::MyData::IsOkay() const
{
    return (mResult != NULL);
}


bool DbMyResult::MyData::FetchRow(DbResultRow& row /*OUT*/)
{
    bool ret;
    size_t i;

    row.clear();

    if (mResult == NULL) return false;

    mRow = mysql_fetch_row(mResult);
    ret = (mRow != NULL);
    if (!ret) return false;
    row.reserve(mNumCols);

    for (i=0; i<mNumCols; i++)
    {
        row.push_back(mRow[i]);
    }
    
    return ret;
}


size_t DbMyResult::MyData::GetNumColumns()
{
    return mNumCols;
}


FString DbMyResult::MyData::GetColumnName(size_t i)
{
    if (mResult == NULL) return "";
    MYSQL_FIELD *fields = mysql_fetch_fields(mResult);
    if (fields == NULL) return FString();
    return fields[i].name;
}


size_t DbMyResult::MyData::GetFieldLength(size_t i)
{
    if (mResult == NULL) return 0;
    unsigned long *lengths = mysql_fetch_lengths(mResult);
    if (lengths == NULL) return 0;
    return (size_t)lengths[i];
}

size_t DbMyResult::MyData::GetNumRows()
{
    if (mResult == NULL) return 0;
    return mysql_num_rows(mResult);
}

bool DbMyResult::MyData::Seek(size_t offset)
{
    if (mResult == NULL) return false;
    mysql_data_seek(mResult, offset);
    return true;
}

#endif
#endif
