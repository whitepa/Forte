#ifndef FORTE_NO_DB
#ifndef FORTE_NO_POSTGRESQL

// DbPgResult.cpp
#include "DbPgResult.h"

// DbPgResult class
DbPgResult::DbPgResult(PGresult *result)
:
    DbResult()
{
    PgData *data = new PgData(result);
    *this = data;
}


DbPgResult::operator PGresult*()
{
    PgData *data = dynamic_cast<PgData*>(mData);
    if (data == NULL) return NULL;
    return data->mResult;
}


uint64_t DbPgResult::InsertID()
{
    PgData *data = dynamic_cast<PgData*>(mData);
    if (data == NULL) return 0;
    return data->InsertID();
}


// DbPgResult::PgData class
DbPgResult::PgData::PgData(PGresult *res)
:
    Data()
{
    mResult = res;
    mNumRows = (size_t)PQntuples(mResult);
    mNumCols = (size_t)PQnfields(mResult);
    mCurrentRow = 0;
}


DbPgResult::PgData::~PgData()
{
    if (mResult != NULL) PQclear(mResult);
}


bool DbPgResult::PgData::IsOkay() const
{
    bool ret = false;

    if (mResult != NULL)
    {
        ExecStatusType code = PQresultStatus(mResult);

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


bool DbPgResult::PgData::FetchRow(DbResultRow& row /*OUT*/)
{
    size_t i;
    char *val;

    row.clear();
    if (mCurrentRow >= mNumRows) return false;
    row.reserve(mNumCols);

    for (i=0; i<mNumCols; i++)
    {
        val = PQgetvalue(mResult, mCurrentRow, i);
        if (val[0] == 0 && PQgetisnull(mResult, mCurrentRow, i)) val = NULL;
        row.push_back(val);
    }

    mCurrentRow++;
    return true;
}


size_t DbPgResult::PgData::GetNumColumns()
{
    return mNumCols;
}


FString DbPgResult::PgData::GetColumnName(size_t i)
{
    return PQfname(mResult, (int)i);
}


size_t DbPgResult::PgData::GetFieldLength(size_t i)
{
    return (size_t)PQgetlength(mResult, mCurrentRow, i);
}


uint64_t DbPgResult::PgData::InsertID()
{
    if (mNumRows != 1 || mNumCols != 1) return 0;
    char *val = PQgetvalue(mResult, 0, 0);
    return strtoull(val, NULL, 10);
}

#endif
#endif
