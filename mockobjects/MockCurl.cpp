
#include "MockCurl.h"

using namespace Forte;

FString MockCurl::sLastURL;

void MockCurl::setURL(const FString& url)
{
    sLastURL = url;
    mURL = url;
}

void MockCurl::perform()
{
    // todo: a much better way to do this
    if (mURL == "http://curlfail/path?clusterID=123456&buildID=1")
    {
        FString err(FStringFC(), "CURL_FAIL_XFER|||%s", mErrorBuffer);
        throw CurlException(404, err);
    }
    // don't perform! (but do something)
}
