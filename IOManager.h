#ifndef __IORequest_h__
#define __IORequest_h__

#include "LogManager.h"
#include "SystemCallUtil.h"
#include "Thread.h"
#include "FTrace.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <libaio.h>

namespace Forte
{
    EXCEPTION_CLASS(EIOManager);
    EXCEPTION_SUBCLASS2(EIOManager, EIOSubmitFailed,
                        "Failed to submit IO operation");

    EXCEPTION_SUBCLASS(EIOManager, EIORequest);
    EXCEPTION_SUBCLASS2(EIORequest, EIORequestStillInProgress,
                        "The IO request is still in progress");
    class IOManager;

    class IORequest : public boost::enable_shared_from_this<IORequest>
    {
    public:
        typedef enum {
            READ,
            WRITE
        } OperationType;
        typedef boost::function<void(const IORequest &req)> IOCompletionCallback;

        IORequest(const boost::shared_ptr<IOManager>& ioManager);

        void Begin();
        void SetOp(IORequest::OperationType op);
        void SetBuffer(void *buf, size_t len);
        void SetOffset(off_t off);
        void SetFD(int fd);
        void SetCallback(IOCompletionCallback cb);
        void SetUserData(void *data);
        void * GetUserData(void) const;

        // Called only by IOManager
        void SetRequestNumber(uint64_t reqNum);
        uint64_t GetRequestNumber(void) const;
        struct iocb ** GetIOCBs(void);
        void SetResult(long res);
        long GetResult(void) const;
        void Wait(void);
        bool IsComplete(void) const;

    protected:
        boost::weak_ptr<IOManager> mIOManager;
        struct iocb mIOCB;
        struct iocb *mIOCBs[1];
        IOCompletionCallback mCallback;
        bool mCompleted;
        long mResult;
        void *mData;

        Forte::Mutex mWaitLock;
        Forte::ThreadCondition mWaitCond;
    };


    typedef boost::shared_ptr<IORequest> IORequestPtr;
    typedef std::pair<uint64_t, IORequestPtr> IORequestPair;
    typedef std::map<uint64_t, IORequestPtr> RequestMap;

    class IOManager : public Forte::Thread
    {
    public:
        IOManager(int maxRequests) :
            mCompletionCond(mLock),
            mIOContext(NULL),
            mRequestCounter(0) {
            int err;
            // @TODO check range on maxRequests
            if ((err = io_setup(maxRequests, &mIOContext)) != 0)
                SystemCallUtil::ThrowErrNoException(-err, "io_setup");
            initialized();
        }
        virtual ~IOManager() {
            // @TODO Wait for all IO completion
            deleting();
            int err;
            if ((err = io_destroy(mIOContext)) != 0)
                hlog(HLOG_ERR, "io_destroy: %s", strerror(-err));
        }
        virtual void * run(void) {
            FTRACE;
            struct timespec timeout = Timespec::FromMillisec(200);
            struct io_event events[32];
            while (!IsShuttingDown()) {
                // @TODO correctly handle shutdown with pending requests
                int n = 0;
                memset(events, 0, sizeof(struct io_event)*32);
                n = io_getevents(mIOContext, 1, 32, events, &timeout);
                if (n > 0)
                {
                    hlog(HLOG_DEBUG, "io_getevents returns %d events", n);
                }
                else if (n == -EINTR)
                {
                    // interrupted syscall
                    continue;
                }
                else if (n < 0)
                {
                    hlog(HLOG_ERR, "io_getevents returns %d: %s", n, strerror(-n));
                    usleep(200000);
                    continue;
                }
                for (int i = 0; i < n; ++i)
                {
                    hlog(HLOG_DEBUG, "received IO completion for request %lu",
                         (long)events[i].obj->data);
                    HandleCompletionEvent(&events[i]);
                }
            }
            return NULL;
        }

        boost::shared_ptr<IORequest> NewRequest() {
            // @TODO allocate from a pool
            AutoUnlockMutex lock(mLock);
            uint64_t reqnum = mRequestCounter++;
            boost::shared_ptr<IORequest> req(
                new IORequest(
                    boost::dynamic_pointer_cast<IOManager>(shared_from_this())));
            req->SetRequestNumber(reqnum);
            return req;
        }

        // should be called by an IORequst
        void SubmitRequest(const boost::shared_ptr<IORequest> &req) {
            AutoUnlockMutex lock(mLock);
            int res;
            while ((res = io_submit(mIOContext, 1, req->GetIOCBs())) < 0)
            {
                if (-res == EAGAIN)
                {
                    // in this situation the IO context is likely full,
                    // and we need to wait for completion handlers to do
                    // their job.
                    mCompletionCond.Wait();
                    continue;
                }
                else if (-res == EINTR)
                {
                    continue;
                }
                SystemCallUtil::ThrowErrNoException(-res, "io_submit");
            }
            // hlog(HLOG_DEBUG4, "io_submit complete: reqnum=%lu res=%d",
            //      reqnum, res);
            if (res == 0)
                throw EIOSubmitFailed();
            // take ownership of request
            mPendingRequests[req->GetRequestNumber()] = req;
        }

        void HandleCompletionEvent(const struct io_event *event) {
            uint64_t requestNum = (uint64_t)event->obj->data;
            boost::shared_ptr<IORequest> req;
            {
                AutoUnlockMutex lock(mLock);
                mCompletionCond.Signal();
                RequestMap::iterator i = mPendingRequests.find(requestNum);
                if (i == mPendingRequests.end())
                {
                    hlog(HLOG_ERR, "ignoring IO completion event for unknown request %lu",
                         requestNum);
                    return;
                }
                req = (*i).second;
                mPendingRequests.erase(requestNum);
            }
            // Set result, triggering callbacks (from this thread)
            req->SetResult(event->res);
        }

        Forte::Mutex mLock;
        Forte::ThreadCondition mCompletionCond;

        io_context_t mIOContext;

        // Counter for submitted requests
        uint64_t mRequestCounter;

        // map of request number to request
        RequestMap mPendingRequests;
    };

}

#endif
