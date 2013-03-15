
#include "MockCurl.h"

using namespace Forte;

FString MockCurl::sLastURL;

void MockCurl::SetURL(const FString& url)
{
    FTRACE2("%s", url.c_str());

    sLastURL = url;
    mURL = url;
}

void MockCurl::Perform()
{
    FTRACE;

    // todo: a much better way to do this
    if (mURL == "http://curlfail/path?clusterID=123456&buildID=1")
    {
        FString err(FStringFC(), "CURL_FAIL_XFER|||%s", mErrorBuffer);
        throw CurlException(404, err);
    }
    // don't perform! (but do something)

    std::map<FString, FString>::const_iterator it;
    if ((it = mURLToBuffer.find(mURL)) != mURLToBuffer.end())
    {
        hlog(HLOG_DEBUG, "Found mBuf (%s) for URL (%s)",
             (*it).second.c_str(), mURL.c_str());
        mBuf = (*it).second;
    }
    else
    {
        hlog(HLOG_WARN, "No buffer found for URL (%s)", mURL.c_str());
        throw ENoActionDefined(mURL);
    }
}

void MockCurl::SetInternalCB(void)
{
    FTRACE;

    mbUseInternalBuffer = true;
}


void MockCurl::ClearBufferAfterURLPerform(const FString& url)
{
    FTRACE2("%s", url.c_str());
    std::map<FString, FString>::iterator it;
    if ((it = mURLToBuffer.find(mURL)) != mURLToBuffer.end())
    {
        hlog(HLOG_DEBUG, "Erasing %s from map", url.c_str());
        mURLToBuffer.erase(it);
    }
}
void MockCurl::SetBufferAfterURLPerform(const FString& url,
                                        const FString& buffer)
{
    FTRACE2("%s, %s", url.c_str(), buffer.c_str());

    mURLToBuffer[url] = buffer;
}
