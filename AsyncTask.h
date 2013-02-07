#ifndef __Forte__AsyncTask_h__
#define __Forte__AsyncTask_h__

#include "Object.h"
#include "AutoMutex.h"
#include "ThreadCondition.h"
#include "FTrace.h"
#include <boost/function.hpp>

EXCEPTION_CLASS(EAsyncTask);
EXCEPTION_SUBCLASS2(EAsyncTask, EAsyncTaskStillInProgress,
                    "Async Task is still in progress");
EXCEPTION_SUBCLASS2(EAsyncTask, EAsyncTaskUnknownException,
                    "Unknown Exception in Async Task");

// Subclasses of AsyncTask must do the following:

//  Do not start the task until Begin() is called.  Begin is to be
//  implemented in the subclass.  Exceptions during task execution
//  must be captured and reported to the task via setException().
//  Note, this will complete the task and notify waiters.  On task
//  completion, call setResult() with the result of the task.  Bottom
//  line, all tasks MUST call either setResult() or setException().

namespace Forte {

    template <typename ResultType>
    class AsyncTask : public Forte::Object
    {
    public:
        typedef boost::function<void(const AsyncTask<ResultType>& asyncTask)> AsyncTaskCompletionCallback;

        AsyncTask() :
            mCompleted(false),
            mWaitCond(mWaitLock) {}

        virtual ~AsyncTask() {}

        virtual ResultType GetResult(void) const {
            if (!IsComplete())
                throw EAsyncTaskStillInProgress();
            if (mException)
            {
                try
                {
                    boost::rethrow_exception(mException);
                }
                catch (boost::unknown_exception &e)
                {
                    hlog(HLOG_ERR, "unknown exception in task result: %s",
                         boost::diagnostic_information(mException).c_str());
                    boost::throw_exception(EAsyncTaskUnknownException());
                }
            }
            return mResult;
        }

        void Wait(void) {
            Forte::AutoUnlockMutex lock(mWaitLock);
            while (!IsComplete())
                mWaitCond.Wait();
        }

        bool IsComplete(void) const { return mCompleted; }

        void SetCallback(AsyncTaskCompletionCallback cb) {
            mCompletionCallback = cb;
        }

        virtual void Begin(void) = 0;

    protected:
        virtual void setResult(ResultType res) {
            {
                AutoUnlockMutex lock(mWaitLock);
                mResult = res;
                mCompleted = true;
                mWaitCond.Broadcast();
                if (mCompletionCallback)
                {
                    try
                    {
                        mCompletionCallback(*this);
                    }
                    catch (...)
                    {
                    }
                    mCompletionCallback = AsyncTaskCompletionCallback();
                }
            }
        }

        virtual void setException(boost::exception_ptr e) {
            FTRACE;
            {
                AutoUnlockMutex lock(mWaitLock);
                mException = e;
                mCompleted = true;
                mWaitCond.Broadcast();
                if (mCompletionCallback)
                {
                    try
                    {
                        mCompletionCallback(*this);
                    }
                    catch (...)
                    {
                    }
                    mCompletionCallback = AsyncTaskCompletionCallback();
                }
            }
        }

    private:
        bool mCompleted;
        ResultType mResult;
        boost::exception_ptr mException;
        Forte::Mutex mWaitLock;
        Forte::ThreadCondition mWaitCond;
        AsyncTaskCompletionCallback mCompletionCallback;
    };

    template <>
    class AsyncTask<void> : public Forte::Object
    {
    public:
        typedef boost::function<void(const AsyncTask<void> &asyncTask)> AsyncTaskCompletionCallback;

        AsyncTask() :
            mCompleted(false),
            mWaitCond(mWaitLock) {}

        virtual ~AsyncTask() {}

        virtual void GetResult(void) const {
            if (!IsComplete())
                throw EAsyncTaskStillInProgress();
            if (mException)
            {
                try
                {
                    boost::rethrow_exception(mException);
                }
                catch (boost::unknown_exception &e)
                {
                    hlog(HLOG_ERR, "unknown exception in task result: %s",
                         boost::diagnostic_information(mException).c_str());
                    boost::throw_exception(EAsyncTaskUnknownException());
                }
            }
        }

        void Wait(void) {
            Forte::AutoUnlockMutex lock(mWaitLock);
            while (!IsComplete())
                mWaitCond.Wait();
        }

        bool IsComplete(void) const { return mCompleted; }

        void SetCallback(AsyncTaskCompletionCallback cb) {
            mCompletionCallback = cb;
        }

        virtual void Begin(void) = 0;

    protected:
        virtual void setResult(void) {
            {
                AutoUnlockMutex lock(mWaitLock);
                mCompleted = true;
                mWaitCond.Broadcast();
                if (mCompletionCallback)
                {
                    try
                    {
                        mCompletionCallback(*this);
                    }
                    catch (...)
                    {
                    }
                    mCompletionCallback = AsyncTaskCompletionCallback();
                }
            }
        }

        virtual void setException(boost::exception_ptr e) {
            FTRACE;
            {
                AutoUnlockMutex lock(mWaitLock);
                mException = e;
                mCompleted = true;
                mWaitCond.Broadcast();
                if (mCompletionCallback)
                {
                    try
                    {
                        mCompletionCallback(*this);
                    }
                    catch (...)
                    {
                    }
                    mCompletionCallback = AsyncTaskCompletionCallback();
                }
            }
        }

    private:
        bool mCompleted;
        boost::exception_ptr mException;
        Forte::Mutex mWaitLock;
        Forte::ThreadCondition mWaitCond;
        AsyncTaskCompletionCallback mCompletionCallback;
    };
}

#endif
