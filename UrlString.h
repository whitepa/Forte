#ifndef __url_string_h__
#define __url_string_h__

#include "FString.h"

class CUrlString {
public:
    static FString & encode(FString &encoded, const FString &decoded);
    static FString & decode(FString &decoded, const FString &encoded);

    // utilities:
    static char charFromHexPair(char hi, char lo);

protected:
    static FString sUnsafe;
};


#endif
