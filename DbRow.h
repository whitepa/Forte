#ifndef DbRow_h
#define DbRow_h

#ifndef FORTE_NO_DB

#include <vector>
#include "Exception.h"

EXCEPTION_SUBCLASS(CForteException, CForteDbRowException);

typedef std::vector<const char*> CDbResultRow;

// base class for a row
class CDbRow {
public:
    CDbRow() { }
    virtual ~CDbRow() { }
    virtual void Set(const CDbResultRow& row) = 0;

protected:
    static void CheckRange(const CDbResultRow& row, int index);
    static bool IsNull(const CDbResultRow& row, int index);
    static int GetInt(const CDbResultRow& row, int index);
    static unsigned int GetUInt(const CDbResultRow& row, int index);
    static unsigned long long GetULLInt(const CDbResultRow& row, int index);
    static bool GetBool(const CDbResultRow& row, int index);
    static FString GetString(const CDbResultRow& row, int index);
    static float GetFloat(const CDbResultRow& row, int index);
    static double GetDouble(const CDbResultRow& row, int index);
};

#endif
#endif
