#ifndef FORTE_NO_DB

// DbRow.cpp

#include "Forte.h"
#include "DbRow.h"

void CDbRow::CheckRange(const CDbResultRow& row, int index)
{
    if (index < 0 || index >= (int)row.size())
        throw CForteDbRowException(FStringFC(), "Software Error: index %d out of range", index);
}

bool CDbRow::IsNull(const CDbResultRow& row, int index)
{
    CheckRange(row, index);
    return (row[index] == NULL);
}

int CDbRow::GetInt(const CDbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0;
    return strtol(row[index], NULL, 10);
}

unsigned int CDbRow::GetUInt(const CDbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0;
    return strtoul(row[index], NULL, 10);
}

unsigned long long CDbRow::GetULLInt(const CDbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0;
    return strtoull(row[index], NULL, 10);
}

bool CDbRow::GetBool(const CDbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return false;
    return atoi(row[index]);
}

FString CDbRow::GetString(const CDbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return "";
    return row[index];
}

float CDbRow::GetFloat(const CDbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0.f;
    return strtof(row[index], NULL);
}

double CDbRow::GetDouble(const CDbResultRow& row, int index)
{
    CheckRange(row, index);
    if (row[index] == NULL) return 0.;
    return strtod(row[index], NULL);
}

#endif
