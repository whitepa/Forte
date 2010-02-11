// GUID.h
#ifndef __forte_guid_h__
#define __forte_guid_h__

#include "FString.h"
namespace Forte
{
    class GUID
    {
    public:
        static FString GenerateGUID(bool pathSafe = false);
    };
};
#endif
