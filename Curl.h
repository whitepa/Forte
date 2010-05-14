#ifndef __forte_Curl_h
#define __forte_Curl_h
#ifndef FORTE_NO_CURL

#include "Types.h"
#include "Object.h"
#include <curl/curl.h>
#include "FileSystem.h"

namespace Forte
{
    class Curl : public Object
    {
    public:
        // types
        class CurlException : public Exception
        {
        public:
            CurlException(int errorCode, const FString &str) :
                Exception(str),
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
        virtual void SetURL(const FString& url);
        void SetRecvHeaderCB(header_cb_t func, void *data = NULL);
        void SetRecvCB(data_cb_t func, void *data = NULL);
        void SetInternalCB(void);
        void SetOutputFile(FILE* file);
        void SetSendCB(data_cb_t func, void *data = NULL);
        void SetProgressCB(progress_cb_t func, void *data = NULL);
        void SetFollowRedirects(bool follow = true);
        void SetRecvSpeedMax(curl_off_t limit = -1);
        void SetSendSpeedMax(curl_off_t limit = -1);
        void SetRecvHeader(bool recvHeader = false);
        void SetNoSignal(bool nosignal = true);
        void SetConnectTimeout(long timeout);
        void SetMaxTransferTime(long max_xfer_time);
        void SetThrowOnHTTPError(bool shouldThrow = true);

        /// Set a low speed threshold, under which the transfer will be aborted.
        void SetLowSpeed(int bps, int time);

        void Reset();
        void Perform();

        FString mBuf;
     protected:
        //TODO: pull this from the application context
        FileSystem mFileSystem;

        CURL *mHandle;
        static bool sDidInit;
        char mErrorBuffer[CURL_ERROR_SIZE + 1];

        void checkResolvConf();
        FString mURL; // this needs to be saved for curl
        bool mThrowOnHTTPError;
        time_t mLastMtime;
    };


    // global initialization helper class
    class CurlInitializer : public Object
    {
    public:
        CurlInitializer(long flags = CURL_GLOBAL_ALL) { Curl::Init(flags); }
        ~CurlInitializer() { Curl::Cleanup(); }
    };


    typedef boost::shared_ptr<Curl> CurlSharedPtr;
};
#endif // FORTE_NO_CURL
#endif // __forte_Curl_h
