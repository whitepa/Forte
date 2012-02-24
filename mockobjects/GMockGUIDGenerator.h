#ifndef __gmockguidgenerator_h
#define __gmockguidgenerator_h

#include "GUIDGenerator.h"

namespace Forte
{
    class GMockGUIDGenerator : public Forte::GUIDGenerator
    {
    public:
        MOCK_METHOD1(GenerateGUID, Forte::FString&(Forte::FString&));
    };
};
#endif
