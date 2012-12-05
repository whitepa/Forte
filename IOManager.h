#ifndef __IORequest_h__
#define __IORequest_h__

#include "SystemCallUtil.h"
#include "Thread.h"
#include "LogManager.h"
#include "FTrace.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace Forte
{
    EXCEPTION_CLASS(EIOManager);
    EXCEPTION_SUBCLASS2(EIOManager, EIOSubmitFailed,
                        "Failed to submit IO operation");

    EXCEPTION_SUBCLASS(EIOManager, EIORequest);
    EXCEPTION_SUBCLASS2(EIORequest, EIORequestStillInProgress,
                        "The IO request is still in progress");
    class IORequest
    {
    public:
        typedef enum {
            READ,
            WRITE
        } OperationType;
        typedef boost::function<void(const IORequest &req)> IOCompletionCallback;

        IORequest() :
//            mIOManager(ioManager),
            mCompleted(false),
            mResult(0),
            mData(NULL),
            mWaitCond(mWaitLock) {
            memset(&mIOCB, 0, sizeof(struct iocb));
            mIOCBs[0] = &mIOCB;
        }
        void SetOp(IORequest::OperationType op) {
            switch (op) {
            case READ:
                mIOCB.aio_lio_opcode = IO_CMD_PREAD;
                break;
            case WRITE:
                mIOCB.aio_lio_opcode = IO_CMD_PWRITE;
                break;
            default:
                break;
            }
        }
        void SetBuffer(void *buf, size_t len) {
            FTRACE2("buf=%p len=%lu", buf, len);
            mIOCB.u.c.buf = buf;
            mIOCB.u.c.nbytes = len;
        }
        void SetOffset(off_t off) {
            mIOCB.u.c.offset = off;
        }
        void SetFD(int fd) {
            mIOCB.aio_fildes = fd;
        }
        void SetCallback(IOCompletionCallback cb) {
            mCallback = cb;
        }
        void SetUserData(void *data) {
            mData = data;
        }
        void * GetUserData(void) const {
            return mData;
        }
        // Called only by IOManager
        void SetRequestNumber(uint64_t reqNum) {
            mIOCB.data = (void *)reqNum;
        }

        uint64_t GetRequestNumber(void) const {
            return (uint64_t)mIOCB.data;
        }

        struct iocb ** GetIOCBs(void) {
            return mIOCBs;
        }
        void SetResult(long res) {
            FTRACE2("res=%ld", res);
            AutoUnlockMutex lock(mWaitLock);
            mResult = res;
            mCompleted = true;
            mWaitCond.Broadcast();
            if (mCallback)
                mCallback(*this);
        }
        long GetResult(void) const {
            if (!mCompleted)
                throw EIORequestStillInProgress();
            return mResult;
        }

        void Wait(void) {
            // wait for the IO completion 

            // @TODO look into the efficiency of using pthread
            // condition variables for this.  We may want to switch to
            // a futex.  In particular, the current implementation
            // instantiates and initializes a pthread_mutex and
            // pthread_cond_t for each IO request.

            Forte::AutoUnlockMutex lock(mWaitLock);
            while (!mCompleted)
                mWaitCond.Wait();
        }
        bool IsComplete(void) const {
            return mCompleted;
        }

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
            mIOContext(NULL),
            mRequestCounter(0) {
            int err;
            // @TODO check range on maxRequests
            if ((err = io_setup(maxRequests, &mIOContext)) != 0)
                SystemCallUtil::ThrowErrNoException(-err);
//            mIOCB = new struct iocb[maxRequests];
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
                else if (n < 0)
                {
                    hlog(HLOG_ERR, "io_getevents returns %d: %s", n, strerror(-n));
                    InterruptibleSleep(Timespec::FromMillisec(200));
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
            return boost::make_shared<IORequest>();
        }

        uint64_t SubmitRequest(const boost::shared_ptr<IORequest> &req) {
            // take ownership of request
            AutoUnlockMutex lock(mLock);
            req->SetRequestNumber(mRequestCounter);
            int res;
            while ((res = io_submit(mIOContext, 1, req->GetIOCBs())) < 0)
            {
                if (-res == EAGAIN)
                    continue;
                SystemCallUtil::ThrowErrNoException(-res);
            }
            if (res == 0)
                throw EIOSubmitFailed();
            mPendingRequests[mRequestCounter] = req;
            return mRequestCounter++;
        }

        void HandleCompletionEvent(const struct io_event *event) {
            uint64_t requestNum = (uint64_t)event->obj->data;
            boost::shared_ptr<IORequest> req;
            {
                AutoUnlockMutex lock(mLock);
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

        io_context_t mIOContext;

        // Counter for submitted requests
        uint64_t mRequestCounter;

        // map of request number to request
        RequestMap mPendingRequests;
    };

}

#endif
