#ifndef _MockCurl_h
#define _MockCurl_h
#ifndef FORTE_NO_CURL

#include "Forte.h"
#include "Curl.h"

namespace Forte
{
    class MockCurl : public Curl
    {
    public:
        // ctor/dtor
        MockCurl(Context &context) : Curl(context) {}
        ~MockCurl() {}

        void setURL(const FString& url);
        void perform();

        static FString sLastURL;
    };
}
#endif // FORTE_NO_CURL
#endif // __forte_Curl_h
