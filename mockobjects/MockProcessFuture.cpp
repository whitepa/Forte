#include "MockProcessFuture.h"

using namespace Forte;



void MockProcessFuture::GetResult()
{
    FTRACE;
    if (!HasStarted())
        throw_exception(EProcessFutureNotRunning());
    Timespec elapsed = Forte::MonotonicClock().GetTime() - mStartTime;
    Timespec remaining = mResponse->mDuration - elapsed;
    if (remaining.IsPositive())
        usleep((remaining.AsMillisec() + 10) * 1000); // 10 extra ms
    if (mResponse->mResponseCode != 0)
        throw_exception(EProcessFutureTerminatedWithNonZeroStatus());
}
void MockProcessFuture::GetResultTimed(const Timespec &timeout)
{
    FTRACE;
    if (!timeout.IsPositive())
        return GetResult();

    if (!HasStarted())
        throw_exception(EProcessFutureNotRunning());
    Timespec elapsed = Forte::MonotonicClock().GetTime() - mStartTime;
    Timespec remaining = mResponse->mDuration - elapsed;
    if (remaining.IsPositive())
    {
        if (timeout < remaining)
        {
            int usec = timeout.AsMillisec() * 1000;
            hlog(HLOG_DEBUG4, "will sleep until timeout: %d usec", usec);
            usleep(usec + 10000);
            throw_exception(EFutureTimeoutWaitingForResult());
        }
        else
        {
            int usec = remaining.AsMillisec() * 1000;
            hlog(HLOG_DEBUG4, "will sleep until command completion: %d usec", usec);
            usleep(usec + 10000);
        }
    }
    if (mResponse->mResponseCode != 0)
        throw_exception(EProcessFutureTerminatedWithNonZeroStatus());
}

bool MockProcessFuture::IsRunning()
{
    if (!HasStarted())
        return false;

    Timespec elapsed = Forte::MonotonicClock().GetTime() - mStartTime;
    if (mResponse->mDuration <= elapsed)
        return false;
    else
    {
        hlog(HLOG_DEBUG4, "%lld ms remaining", (mResponse->mDuration - elapsed).AsMillisec());
        return true;
    }
}

unsigned int MockProcessFuture::GetStatusCode()
{
    if (IsRunning())
        throw_exception(EProcessFutureNotFinished());
    else
        return mResponse->mResponseCode;
}

FString MockProcessFuture::GetOutputString()
{
    if (IsRunning())
        throw_exception(EProcessFutureNotFinished());
    else
        return mResponse->mResponse;
}

FString MockProcessFuture::GetErrorString()
{
    if (IsRunning())
        throw_exception(EProcessFutureNotFinished());
    else
        return mResponse->mErrorResponse;
}

void MockProcessFuture::SetErrorFilename(const FString &errorfile)
{
    
}
