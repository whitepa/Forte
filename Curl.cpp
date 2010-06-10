// Curl.cpp
#ifndef FORTE_NO_CURL
#include "Forte.h"

using namespace Forte;

#include <resolv.h>

// macros
#define SET_CURL_OPT(OPTION, VALUE)                                     \
    {                                                                   \
        int errCode = curl_easy_setopt(mHandle, OPTION, VALUE);        \
        if (errCode != CURLE_OK)                                        \
        {                                                               \
            FString err(FStringFC(),                                    \
                        "CURL_FAIL_SETOPT|||%s|||%s",                   \
                        #OPTION, mErrorBuffer);                       \
            throw CurlException(errCode, err);                          \
        }                                                               \
    }


// statics
bool Curl::sDidInit = false;

static size_t curlInternalDataCB(void *buffer, size_t size,
                                 size_t nmemb, void *userp)
{
    if (userp != NULL)
    {
        ((Curl*) userp)->mBuf.append((char*) buffer, size * nmemb);
        return size * nmemb;
    }
    return 0;
}

void Curl::SetInternalCB(void)
{
    SetRecvCB(curlInternalDataCB, this);
}

void Curl::Init(long flags)
{
    sDidInit = true;
}


void Curl::Cleanup()
{
    sDidInit = false;
}


// class methods
Curl::Curl() :
    mHandle(NULL), mThrowOnHTTPError(false)
{
    if (!sDidInit)
    {
        throw CurlException(0, "Failed to call curl_global_init");
    }

    // load resolv.conf once per thread
    mLastMtime = 0;

    // get curl handle
    if ((mHandle = curl_easy_init()) == NULL)
    {
        throw CurlException(CURLE_FAILED_INIT, "CURL_FAIL_INIT");
    }

    memset(mErrorBuffer, 0, sizeof(mErrorBuffer));
    if (curl_easy_setopt(mHandle, CURLOPT_ERRORBUFFER, mErrorBuffer) != CURLE_OK)
    {
        throw CurlException(CURLE_FAILED_INIT, "CURL_FAIL_INIT");
    }

    SetRecvHeader(false); // no header in the body by default
}


Curl::~Curl()
{
    if (mHandle != NULL) curl_easy_cleanup(mHandle);
}


void Curl::SetURL(const FString& url)
{
    mURL = url;
    SET_CURL_OPT(CURLOPT_URL, mURL.c_str());
}

void Curl::SetRecvHeaderCB(header_cb_t func, void *data)
{
    SET_CURL_OPT(CURLOPT_HEADERFUNCTION, func);
    SET_CURL_OPT(CURLOPT_HEADERDATA, data);
}


void Curl::SetRecvCB(data_cb_t func, void *data)
{
    SET_CURL_OPT(CURLOPT_WRITEFUNCTION, func);
    SET_CURL_OPT(CURLOPT_WRITEDATA, data);
}

void Curl::SetOutputFile(FILE* file)
{
    SET_CURL_OPT(CURLOPT_WRITEFUNCTION, NULL);
    SET_CURL_OPT(CURLOPT_WRITEDATA, file);
}

void Curl::SetSendCB(data_cb_t func, void *data)
{
    SET_CURL_OPT(CURLOPT_READFUNCTION, func);
    SET_CURL_OPT(CURLOPT_READDATA, data);
}


void Curl::SetProgressCB(progress_cb_t func, void *data)
{
    SET_CURL_OPT(CURLOPT_PROGRESSFUNCTION, func);
    SET_CURL_OPT(CURLOPT_PROGRESSDATA, data);
}


void Curl::SetFollowRedirects(bool follow)
{
    SET_CURL_OPT(CURLOPT_FOLLOWLOCATION, (follow ? 1 : 0));
}


void Curl::SetRecvSpeedMax(curl_off_t limit)
{
    SET_CURL_OPT(CURLOPT_MAX_RECV_SPEED_LARGE, limit);
}


void Curl::SetSendSpeedMax(curl_off_t limit)
{
    SET_CURL_OPT(CURLOPT_MAX_SEND_SPEED_LARGE, limit);
}


void Curl::SetRecvHeader(bool recvHeader)
{
    SET_CURL_OPT(CURLOPT_HEADER, recvHeader ? 1 : 0);
}

void Curl::SetNoSignal(bool nosignal)
{
    SET_CURL_OPT(CURLOPT_NOSIGNAL, nosignal ? 1 : 0);
}


void Curl::SetConnectTimeout(long timeout)
{
    SET_CURL_OPT(CURLOPT_CONNECTTIMEOUT, timeout);
}


void Curl::SetMaxTransferTime(long max_xfer_time)
{
    SET_CURL_OPT(CURLOPT_TIMEOUT, max_xfer_time);
}


/// Set a low speed threshold, under which the transfer will be aborted.
void Curl::SetLowSpeed(int bps, int time)
{
    SET_CURL_OPT(CURLOPT_LOW_SPEED_LIMIT, bps);
    SET_CURL_OPT(CURLOPT_LOW_SPEED_TIME, time);
}

void Curl::SetThrowOnHTTPError(bool shouldThrow)
{
    mThrowOnHTTPError = shouldThrow;
}

void Curl::Reset()
{
    mBuf.clear();
    curl_easy_reset(mHandle);
}

//TODO: move to resolver manager class so this is only done once per thread
void Curl::checkResolvConf()
{
    struct stat st;
    if (mFileSystem.Stat("/etc/resolv.conf", &st) == 0)
    {
        if (st.st_mtime != mLastMtime)
        {
            mLastMtime = st.st_mtime;
            res_nclose(&_res);

            if (res_ninit(&_res) != 0)
            {
                hlog(HLOG_WARN, "res_init failed. "
                     "continuing with cached. errno: %i", errno);
            }
            else
            {
                hlog(HLOG_INFO, "resolv.conf changed. reloaded.");
            }
        }
    }
    else
    {
        hlog(HLOG_WARN, "could not stat /etc/resolv.conf. "
             "continuing with cached. errno: %i", errno);
    }
}

void Curl::Perform()
{
    mBuf.clear();

    checkResolvConf();

    int errCode = curl_easy_perform(mHandle);
    if (errCode != CURLE_OK)
    {
        FString err(FStringFC(), "CURL_FAIL_XFER|||%s|||%s", 
                    mErrorBuffer, mURL.c_str());
        throw CurlException(errCode, err);
    }

    if (mThrowOnHTTPError)
    {
        int httpRetCode = 0;
        curl_easy_getinfo(mHandle, CURLINFO_RESPONSE_CODE, &httpRetCode);

        if (httpRetCode >= 400) // Should we accept 200 only?
        {
            FString err(FStringFC(), "CURL_FAIL_XFER|||%i", httpRetCode);
            throw CurlException(errCode, err);
        }
    }
}

#endif  // FORTE_NO_CURL
