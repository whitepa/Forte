#ifndef DbRow_h
#define DbRow_h

#ifndef FORTE_NO_DB

#include <vector>
#include "FString.h"
#include "Exception.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, ForteDbRowException);

    typedef std::vector<const char*> DbResultRow;

// base class for a row
    class DbRow {
    public:
        DbRow() { }
        virtual ~DbRow() { }
        virtual void Set(const DbResultRow& row) = 0;

    protected:
        static void CheckRange(const DbResultRow& row, int index);
        static bool IsNull(const DbResultRow& row, int index);
        static int GetInt(const DbResultRow& row, int index);
        static unsigned int GetUInt(const DbResultRow& row, int index);
        static long long GetLLInt(const DbResultRow& row, int index);
        static unsigned long long GetULLInt(const DbResultRow& row, int index);
        static bool GetBool(const DbResultRow& row, int index);
        static FString GetString(const DbResultRow& row, int index);
        static float GetFloat(const DbResultRow& row, int index);
        static double GetDouble(const DbResultRow& row, int index);
    };
};
#endif
#endif
