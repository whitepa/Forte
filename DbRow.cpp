#ifndef FORTE_NO_DB

// DbRow.cpp

#include "DbRow.h"

using namespace Forte;

void DbRow::CheckRange(const DbResultRow& row, int index)
{
    if (index < 0 || index >= (int)row.size())
        throw ForteDbRowException(FStringFC(), "Software Error: index %d out of range", index);
}

bool DbRow::IsNull(const DbResultRow& row, int index)
{
    CheckRange(row, index);
    return (row[index] == NULL);
}

int DbRow::GetInt(const DbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0;
    return strtol(row[index], NULL, 10);
}

unsigned int DbRow::GetUInt(const DbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0;
    return strtoul(row[index], NULL, 10);
}

long long DbRow::GetLLInt(const DbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0;
    return strtoll(row[index], NULL, 10);
}

unsigned long long DbRow::GetULLInt(const DbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0;
    return strtoull(row[index], NULL, 10);
}

bool DbRow::GetBool(const DbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return false;
    return atoi(row[index]);
}

FString DbRow::GetString(const DbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return "";
    return row[index];
}

float DbRow::GetFloat(const DbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0.f;
    return strtof(row[index], NULL);
}

double DbRow::GetDouble(const DbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0.;
    return strtod(row[index], NULL);
}

#endif
