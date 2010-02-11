// Random.h
#ifndef __forte_random_h__
#define __forte_random_h__

#include "FString.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(CForteException, CForteRandomException);

    class Random
    {
    public:
        static FString GetSecureRandomData(unsigned int length);
        static FString GetRandomData(unsigned int length);
        static unsigned int GetRandomUInt(void);
    };
};
#endif
