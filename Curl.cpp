// Curl.cpp
#ifndef FORTE_NO_CURL
#include "Forte.h"

// macros
#define SET_CURL_OPT(OPTION, VALUE)                                     \
    {                                                                   \
        int errCode = curl_easy_setopt(m_handle, OPTION, VALUE);        \
        if (errCode != CURLE_OK)                                        \
        {                                                               \
            FString err(FStringFC(),                                    \
                        "CURL_FAIL_SETOPT|||%s|||%s",                   \
                        #OPTION, m_error_buffer);                       \
            throw CurlException(errCode, err);                          \
        }                                                               \
    }


// statics
bool Curl::s_did_init = false;

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

void Curl::setInternalCB(void)
{
    setRecvCB(curlInternalDataCB, this);
}

void Curl::Init(long flags)
{
    if (!s_did_init)
    {
        s_did_init = true;
        curl_global_init(flags);
    }
}


void Curl::Cleanup()
{
    if (s_did_init)
    {
        s_did_init = false;
        curl_global_cleanup();
    }
}


// class methods
Curl::Curl() :
    m_handle(NULL)
{
    // get curl handle
    if ((m_handle = curl_easy_init()) == NULL)
    {
        throw CurlException(CURLE_FAILED_INIT, "CURL_FAIL_INIT");
    }

    if (curl_easy_setopt(m_handle, CURLOPT_ERRORBUFFER, m_error_buffer) != CURLE_OK)
    {
        throw CurlException(CURLE_FAILED_INIT, "CURL_FAIL_INIT");
    }

    memset(m_error_buffer, 0, sizeof(m_error_buffer));
    setRecvHeader(false); // no header in the body by default
}


Curl::~Curl()
{
    if (m_handle != NULL) curl_easy_cleanup(m_handle);
}


void Curl::setURL(const FString& url)
{
    SET_CURL_OPT(CURLOPT_URL, url.c_str());
}


void Curl::setRecvHeaderCB(header_cb_t func, void *data)
{
    SET_CURL_OPT(CURLOPT_HEADERFUNCTION, func);
    SET_CURL_OPT(CURLOPT_HEADERDATA, data);
}


void Curl::setRecvCB(data_cb_t func, void *data)
{
    SET_CURL_OPT(CURLOPT_WRITEFUNCTION, func);
    SET_CURL_OPT(CURLOPT_WRITEDATA, data);
}

void Curl::setOutputFile(FILE* file)
{
    SET_CURL_OPT(CURLOPT_WRITEFUNCTION, NULL);
    SET_CURL_OPT(CURLOPT_WRITEDATA, file);
}

void Curl::setSendCB(data_cb_t func, void *data)
{
    SET_CURL_OPT(CURLOPT_READFUNCTION, func);
    SET_CURL_OPT(CURLOPT_READDATA, data);
}


void Curl::setProgressCB(progress_cb_t func, void *data)
{
    SET_CURL_OPT(CURLOPT_PROGRESSFUNCTION, func);
    SET_CURL_OPT(CURLOPT_PROGRESSDATA, data);
}


void Curl::setFollowRedirects(bool follow)
{
    SET_CURL_OPT(CURLOPT_FOLLOWLOCATION, (follow ? 1 : 0));
}


void Curl::setRecvSpeedMax(curl_off_t limit)
{
    SET_CURL_OPT(CURLOPT_MAX_RECV_SPEED_LARGE, limit);
}


void Curl::setSendSpeedMax(curl_off_t limit)
{
    SET_CURL_OPT(CURLOPT_MAX_SEND_SPEED_LARGE, limit);
}


void Curl::setRecvHeader(bool recvHeader)
{
    SET_CURL_OPT(CURLOPT_HEADER, recvHeader ? 1 : 0);
}

void Curl::setNoSignal(bool nosignal)
{
    SET_CURL_OPT(CURLOPT_NOSIGNAL, nosignal ? 1 : 0);
}


void Curl::setConnectTimeout(long timeout)
{
    SET_CURL_OPT(CURLOPT_CONNECTTIMEOUT, timeout);
}


void Curl::setMaxTransferTime(long max_xfer_time)
{
    SET_CURL_OPT(CURLOPT_TIMEOUT, max_xfer_time);
}


/// Set a low speed threshold, under which the transfer will be aborted.
void Curl::setLowSpeed(int bps, int time)
{
    SET_CURL_OPT(CURLOPT_LOW_SPEED_LIMIT, bps);
    SET_CURL_OPT(CURLOPT_LOW_SPEED_TIME, time);
}


void Curl::reset()
{
    mBuf.clear();
    curl_easy_reset(m_handle);
}


void Curl::perform()
{
    mBuf.clear();
    int errCode = curl_easy_perform(m_handle);
    if (errCode != CURLE_OK)
    {
        FString err(FStringFC(), "CURL_FAIL_XFER|||%s", m_error_buffer);
        throw CurlException(errCode, err);
    }
}

#endif  // FORTE_NO_CURL
