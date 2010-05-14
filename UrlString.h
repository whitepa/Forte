#ifndef __url_string_h__
#define __url_string_h__

#include "FString.h"
namespace Forte
{
    class UrlString {
    public:
        static FString & Encode(FString &encoded, const FString &decoded);
        static FString & Decode(FString &decoded, const FString &encoded);

        // utilities:
        static char CharFromHexPair(char hi, char lo);

     protected:
        static FString sUnsafe;
    };
};

#endif
