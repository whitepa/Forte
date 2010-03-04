#ifndef __forte_Curl_h
#define __forte_Curl_h
#ifndef FORTE_NO_CURL

#include "Types.h"
#include "Object.h"
#include <curl/curl.h>

namespace Forte
{
    class Curl : public Object
    {
    public:
        // types
        class CurlException : public CException
        {
        public:
            CurlException(int errorCode, const FString &str) :
                CException(str),
                mErrorCode(errorCode) {};
            int mErrorCode;
        };

        typedef size_t (*header_cb_t)(void *buffer, size_t size, size_t nmemb, void *userp);
        typedef size_t (*data_cb_t)(void *buffer, size_t size, size_t nmemb, void *userp);
        typedef int (*progress_cb_t)(void *clientp, double dltotal, double dlnow,
                                     double ultotal, double ulnow);

    public:
        // static
        static void Init(long flags = CURL_GLOBAL_ALL);
        static void Cleanup();

    public:
        // ctor/dtor
        Curl();
        ~Curl();

        // interface
        void setURL(const FString& url);
        void setRecvHeaderCB(header_cb_t func, void *data = NULL);
        void setRecvCB(data_cb_t func, void *data = NULL);
        void setInternalCB(void);
        void setOutputFile(FILE* file);
        void setSendCB(data_cb_t func, void *data = NULL);
        void setProgressCB(progress_cb_t func, void *data = NULL);
        void setFollowRedirects(bool follow = true);
        void setRecvSpeedMax(curl_off_t limit = -1);
        void setSendSpeedMax(curl_off_t limit = -1);
        void setRecvHeader(bool recvHeader = false);
        void setNoSignal(bool nosignal = true);
        void setConnectTimeout(long timeout);
        void setMaxTransferTime(long max_xfer_time);

        /// Set a low speed threshold, under which the transfer will be aborted.
        void setLowSpeed(int bps, int time);

        void reset();
        void perform();

        FString mBuf;
    protected:
        CURL *m_handle;
        static bool s_did_init;
        char m_error_buffer[CURL_ERROR_SIZE + 1];
    };


// global initialization helper class
    class CurlInitializer : public Object
    {
    public:
        CurlInitializer(long flags = CURL_GLOBAL_ALL) { Curl::Init(flags); }
        ~CurlInitializer() { Curl::Cleanup(); }
    };
};
#endif // FORTE_NO_CURL
#endif // __forte_Curl_h
