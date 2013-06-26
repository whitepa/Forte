#include "EPollMonitor.h"
#include <sys/types.h>
#include <boost/bind.hpp>
#include "SystemCallUtil.h"
#include "FunctionThread.h"

using namespace Forte;

EPollMonitor::EPollMonitor(const std::string& name, int epollTimeoutMs)
    : mName(name),
      mEPollTimeoutMs(epollTimeoutMs)
{
    FTRACE;
    mEPollFD = epoll_create1(0);
    if (mEPollFD == -1)
    {
        hlog_and_throw(HLOG_WARN, EPollMonitorCreateEPollFD(
                           FStringFC(), "%d", errno));
    }
}

EPollMonitor::~EPollMonitor()
{
    FTRACE;
    if (mFDToCallbackMap.size() != 0)
    {
        hlogstream(HLOG_ERR,"EPollMonitor being deleted with "
                   << mFDToCallbackMap.size() << " fds");
    }
    close(mEPollFD);
}

void EPollMonitor::AddFD(int fd,
                         struct epoll_event& ev,
                         EPollEventHandler handler)
{
    FTRACE2("%d", fd);

    AutoUnlockMutex lock(mFDMutex);

    if (!mFDToCallbackMap.insert(std::make_pair(fd, handler)).second)
    {
        hlog_and_throw(HLOG_WARN, EEPollMonitorDuplicateFD());
    }

    if (epoll_ctl(mEPollFD, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        hlog_and_throw(
            HLOG_WARN, EEPollMonitorAddFD(
                FStringFC(),
                "%i:%s",
                errno,
                SystemCallUtil::GetErrorDescription(errno).c_str()));
    }
}

void EPollMonitor::ModFD(int fd, struct epoll_event& ev)
{
    FTRACE2("%d", fd);

    AutoUnlockMutex lock(mFDMutex);

    if (epoll_ctl(mEPollFD, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        hlog_and_throw(
            HLOG_WARN, EEPollMonitorModFD(
                FStringFC(),
                "%i:%s",
                errno,
                SystemCallUtil::GetErrorDescription(errno).c_str()));
    }
}

void EPollMonitor::RemoveFD(int fd)
{
    FTRACE2("%d", fd);

    AutoUnlockMutex lock(mFDMutex);

    if (mFDToCallbackMap.find(fd) == mFDToCallbackMap.end())
    {
        hlog(HLOG_ERR, "request to remove invalid fd %d", fd);
    }

    //closing fd removes it from epoll
    if (epoll_ctl(mEPollFD, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        hlog(HLOG_WARN, "EPOLL_CTL_DEL failed: %s",
             SystemCallUtil::GetErrorDescription(errno).c_str());
    }

    if (mFDToCallbackMap.erase(fd) != 1)
    {
        hlog(HLOG_ERR, "Removed invalid fd from map");
    }
}

void EPollMonitor::Start()
{
    recordStartCall();
    mMonitorThread.reset(
        new Forte::FunctionThread(
            Forte::FunctionThread::AutoInit(),
            boost::bind(&EPollMonitor::monitorThreadRun, this),
            mName + "-epl"));
}

void EPollMonitor::Shutdown()
{
    recordShutdownCall();
    mMonitorThread->Shutdown();
    mMonitorThread->WaitForShutdown();

    AutoUnlockMutex lock(mFDMutex);
    std::map<int, EPollEventHandler>::iterator fdIt;

    for(fdIt=mFDToCallbackMap.begin(); fdIt!=mFDToCallbackMap.end(); ++fdIt)
    {
        if (epoll_ctl(mEPollFD, EPOLL_CTL_DEL, fdIt->first, NULL) == -1)
        {
            hlog(HLOG_WARN, "EPOLL_CTL_DEL failed: %s",
                 SystemCallUtil::GetErrorDescription(errno).c_str());
        }
    }
    mFDToCallbackMap.clear();

    mMonitorThread.reset();
}

void EPollMonitor::monitorThreadRun()
{
    int fdReadyCount = 0;
    struct epoll_event events[32];
    Thread* thisThread = Thread::MyThread();
    EPollEventHandler handler;

    while (!thisThread->IsShuttingDown())
    {
        memset(events, 0, 32*sizeof(struct epoll_event));
        fdReadyCount = epoll_wait(mEPollFD, events, 32, mEPollTimeoutMs);
        //REVIEW: should this loop exit on -1?
        if (fdReadyCount < 0 && errno != EINTR && hlog_ratelimit(60))
            hlogstream(HLOG_WARN, "error from epoll thread: " << errno);

        for (int i=0; i<fdReadyCount; i++)
        {
            try
            {
                {
                    AutoUnlockMutex lock(mFDMutex);
                    handler = mFDToCallbackMap.at(events[i].data.fd);
                }
                handler(events[i]);
            }
            catch(std::exception& e)
            {
                hlogstream(HLOG_WARN, "err from epoll callback " << e.what());
            }
        }
    }
}
