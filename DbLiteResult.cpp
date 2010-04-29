#ifndef FORTE_NO_DB
#ifndef FORTE_NO_SQLITE

// DbLiteResult.cpp
#include "Forte.h"
#include "DbLiteResult.h"
#include "DbLiteConnection.h"

// DbLiteResult class
int DbLiteResult::Load(sqlite3_stmt *stmt)
{
    LiteData *data = dynamic_cast<LiteData*>(mData);

    if (data == NULL)
    {
        data = new LiteData();
        *this = data;
    }

    return data->Load(stmt);
}


// DbLiteResult::LiteData class
DbLiteResult::LiteData::LiteData()
:
    Data()
{
    mOkay = true;
    mCurrentRow = mNextRow = mRes.end();
}


DbLiteResult::LiteData::~LiteData()
{
}


int DbLiteResult::LiteData::Load(sqlite3_stmt *stmt)
{
    int i, n, err, type;
    Val val;
    Row row;

    // clear data
    mOkay = false;  // in case of some exception being thrown
    mColNames.clear();
    mRes.clear();
    n = sqlite3_column_count(stmt);

    // run statement
    while ((err = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        // init row
        row.clear();
        row.reserve(n);

        // fetch values one column at a time
        for (i=0; i<n; i++)
        {
            type = sqlite3_column_type(stmt, i);

            if (type == SQLITE_NULL)
            {
                val.first.clear();
                val.second = true;
            }
            else if (type == SQLITE_BLOB)
            {
                val.first.assign((const char*)sqlite3_column_blob(stmt, i),
                                 sqlite3_column_bytes(stmt, i));
                val.second = false;
            }
            else
            {
                val.first = (const char*)sqlite3_column_text(stmt, i);
                val.second = false;
            }

            row.push_back(val);
        }

        // store row
        mRes.push_back(row);
    }

    // check code
    if (err == SQLITE_DONE) err = SQLITE_OK;

    if (err == SQLITE_OK)
    {
        // load column names
        mColNames.resize(n);

        for (i=0; i<n; i++)
        {
            mColNames.push_back(sqlite3_column_name(stmt, i));
        }

        // okay
        mNextRow = mRes.begin();
        mOkay = true;
    }
    else
    {
        // not okay
        mNextRow = mRes.end();
        mOkay = false;
    }

    // done
    mCurrentRow = mRes.end();
    return err;
}


bool DbLiteResult::LiteData::isOkay() const
{
    return mOkay;
}


bool DbLiteResult::LiteData::fetchRow(DbResultRow& row /*OUT*/)
{
    Row::iterator ri;
    if (mNextRow == mRes.end()) return false;
    mCurrentRow = mNextRow;
    ++mNextRow;
    row.clear();
    row.reserve(mCurrentRow->size());
    
    for (ri = mCurrentRow->begin(); ri != mCurrentRow->end(); ++ri)
    {
        if (ri->second) row.push_back(NULL);
        else row.push_back(ri->first.c_str());
    }

    return true;
}


size_t DbLiteResult::LiteData::getNumColumns()
{
    return mColNames.size();
}


FString DbLiteResult::LiteData::getColumnName(size_t i)
{
    return mColNames[i];
}


size_t DbLiteResult::LiteData::getFieldLength(size_t i)
{
    return (*mCurrentRow)[i].first.size();
}


size_t DbLiteResult::LiteData::getNumRows()
{
    return mRes.size();
}


bool DbLiteResult::LiteData::seek(size_t offset)
{
    size_t i;

    mCurrentRow = mRes.end();
    mNextRow = mRes.begin();

    for (i=0; i<offset && mNextRow != mRes.end(); i++)
    {
        mCurrentRow = mNextRow;
        ++mNextRow;
    }

    return (mNextRow != mRes.end());
}

#endif
#endif
