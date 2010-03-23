#ifndef __base64_h_
#define __base64_h_

#include "FString.h"
#include "Exception.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(ForteException, ForteBase64Exception);

    class Base64 : public Object {
    public:
        static int Encode(const char *data, int size, FString &out);
        static int Decode(const char *in, FString &out);
    };
};

#endif
