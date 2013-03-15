#ifndef _MockCurl_h
#define _MockCurl_h
#ifndef FORTE_NO_CURL

#include "Forte.h"
#include "Curl.h"

EXCEPTION_SUBCLASS(Forte::Curl::CurlException, MockCurlException);
EXCEPTION_SUBCLASS(MockCurlException, ENoActionDefined);

namespace Forte
{
    class MockCurl : public Curl
    {
    public:
        // ctor/dtor
        MockCurl() : Curl(), mbUseInternalBuffer(false) {}
        ~MockCurl() {}

        virtual void SetURL(const FString& url);
        virtual void Perform();
        virtual void SetInternalCB(void);

        /*
         * Sets the buffer that will be set (mBuf) after a .Perform()
         * is invoked on that URL
         * @param url     the expected URL
         * @param buffer  the buffer to set when that URL is seen
         */
        virtual void SetBufferAfterURLPerform(const FString& url,
                                              const FString& buffer);
        virtual void ClearBufferAfterURLPerform(const FString& url);
        std::map<FString, FString> mURLToBuffer;

        static FString sLastURL;
    protected:
        bool mbUseInternalBuffer;
    };
}
#endif // FORTE_NO_CURL
#endif // __forte_Curl_h
