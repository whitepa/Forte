#ifndef __EPollThread_h
#define __EPollThread_h

#include <sys/epoll.h>
#include <sys/types.h>
#include "ThreadedObject.h"
#include "AutoMutex.h"
#include "AutoFD.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

namespace Forte
{
    EXCEPTION_CLASS(EEPollMonitor);
    EXCEPTION_SUBCLASS(EEPollMonitor, EPollMonitorCreateEPollFD);
    EXCEPTION_SUBCLASS(EEPollMonitor, EEPollMonitorDuplicateFD);
    EXCEPTION_SUBCLASS(EEPollMonitor, EEPollMonitorAddFD);
    EXCEPTION_SUBCLASS(EEPollMonitor, EEPollMonitorModFD);

    typedef boost::function<void(const struct epoll_event& e)> EPollEventHandler;

    class EPollMonitor : public ThreadedObject
    {
    public:
        EPollMonitor(const std::string& name="eplmon", int epollTimeoutMs=500);
        virtual ~EPollMonitor();
        void AddFD(int fd,
                   struct epoll_event& ev,
                   EPollEventHandler handler);
        void ModFD(int fd,
                   struct epoll_event& ev);
        void RemoveFD(int fd);
        void Start();
        void Shutdown();

    protected:
        virtual void monitorThreadRun();

    protected:
        const std::string mName;
        const int mEPollTimeoutMs;

        int mEPollFD;
        boost::shared_ptr<Forte::Thread> mMonitorThread;

        Forte::Mutex mFDMutex;
        std::map<int, EPollEventHandler> mFDToCallbackMap;
    };
};
#endif
