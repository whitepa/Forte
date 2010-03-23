#ifndef FORTE_NO_DB
#ifndef FORTE_NO_SQLITE

// DbLiteResult.cpp
#include "Forte.h"
#include "DbLiteResult.h"
#include "DbLiteConnection.h"

// DbLiteResult class
int DbLiteResult::Load(sqlite3_stmt *stmt)
{
    LiteData *data = dynamic_cast<LiteData*>(m_data);

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
    m_okay = true;
    m_current_row = m_next_row = m_res.end();
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
    m_okay = false;  // in case of some exception being thrown
    m_col_names.clear();
    m_res.clear();
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
        m_res.push_back(row);
    }

    // check code
    if (err == SQLITE_DONE) err = SQLITE_OK;

    if (err == SQLITE_OK)
    {
        // load column names
        m_col_names.resize(n);

        for (i=0; i<n; i++)
        {
            m_col_names.push_back(sqlite3_column_name(stmt, i));
        }

        // okay
        m_next_row = m_res.begin();
        m_okay = true;
    }
    else
    {
        // not okay
        m_next_row = m_res.end();
        m_okay = false;
    }

    // done
    m_current_row = m_res.end();
    return err;
}


bool DbLiteResult::LiteData::isOkay() const
{
    return m_okay;
}


bool DbLiteResult::LiteData::fetchRow(DbResultRow& row /*OUT*/)
{
    Row::iterator ri;
    if (m_next_row == m_res.end()) return false;
    m_current_row = m_next_row;
    ++m_next_row;
    row.clear();
    row.reserve(m_current_row->size());
    
    for (ri = m_current_row->begin(); ri != m_current_row->end(); ++ri)
    {
        if (ri->second) row.push_back(NULL);
        else row.push_back(ri->first.c_str());
    }

    return true;
}


size_t DbLiteResult::LiteData::getNumColumns()
{
    return m_col_names.size();
}


FString DbLiteResult::LiteData::getColumnName(size_t i)
{
    return m_col_names[i];
}


size_t DbLiteResult::LiteData::getFieldLength(size_t i)
{
    return (*m_current_row)[i].first.size();
}


size_t DbLiteResult::LiteData::getNumRows()
{
    return m_res.size();
}


bool DbLiteResult::LiteData::seek(size_t offset)
{
    size_t i;

    m_current_row = m_res.end();
    m_next_row = m_res.begin();

    for (i=0; i<offset && m_next_row != m_res.end(); i++)
    {
        m_current_row = m_next_row;
        ++m_next_row;
    }

    return (m_next_row != m_res.end());
}

#endif
#endif
