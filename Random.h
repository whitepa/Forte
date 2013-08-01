// Random.h
#ifndef __forte_random_h__
#define __forte_random_h__

#include "FString.h"
#include "Exception.h"

namespace Forte
{
    EXCEPTION_CLASS(ERandom);

    class Random : public Object
    {
    public:
        static FString GetRandomData(unsigned int length);
        static unsigned int GetRandomUInt(void);
    };
};
#endif
