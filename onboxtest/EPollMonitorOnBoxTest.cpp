// #SCQAD TESTAG: forte
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "FTrace.h"
#include "LogManager.h"

#include "EPollMonitor.h"

#include <boost/bind.hpp>

using namespace std;
using namespace boost;
using namespace Forte;
using ::testing::UnitTest;

LogManager logManager;

class EPollMonitorOnBoxTest : public ::testing::Test
{
public:
    EPollMonitorOnBoxTest()
        : mEventVectorMutex(),
          mEventVectorNotEmptyCondition(mEventVectorMutex)
        {
        }

    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
    }

    static void TearDownTestCase() {
        logManager.EndLogging();
    }

    void SetUp() {
        hlogstream(
            HLOG_INFO, "Starting test "
            << UnitTest::GetInstance()->current_test_info()->name());
    }

    void TearDown() {
        hlogstream(
            HLOG_INFO, "ending test "
            << UnitTest::GetInstance()->current_test_info()->name());
    }

    void handleEPollEvent(const struct epoll_event& ev) {
        cout << "epoll event" << endl;
        AutoUnlockMutex lock(mEventVectorMutex);
        mEventVector.push_back(ev);
        mEventVectorNotEmptyCondition.Signal();

        char c;
        if (ev.events & EPOLLIN)
        {
            cout << "recv: ";
            while (recv(ev.data.fd, &c, 1, MSG_NOSIGNAL|MSG_DONTWAIT) > 0)
            {
                cout << c;
            }
            cout << endl;
        }
    }

    Forte::Mutex mEventVectorMutex;
    Forte::ThreadCondition mEventVectorNotEmptyCondition;
    std::vector<struct epoll_event> mEventVector;
};

TEST_F(EPollMonitorOnBoxTest, ConstructDelete)
{
    EPollMonitor monitor("test");
}

TEST_F(EPollMonitorOnBoxTest, StartShutdown)
{
    EPollMonitor monitor("test");
    monitor.Start();
    monitor.Shutdown();
}

TEST_F(EPollMonitorOnBoxTest, RecvEvent)
{
    EPollMonitor monitor("test");
    monitor.Start();

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fda(fds[0]);
    Forte::AutoFD fdb(fds[1]);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
    ev.data.ptr = NULL;
    ev.data.fd = fds[1];

    monitor.AddFD(
        fds[1],
        ev,
        boost::bind(&EPollMonitorOnBoxTest::handleEPollEvent, this, _1));


    if (send(fds[0], "recv", 4, 0) == -1)
    {
        hlog(HLOG_WARN, "test data didn't send");
    }

    {
        AutoUnlockMutex lock(mEventVectorMutex);
        while (mEventVector.empty())
        {
            mEventVectorNotEmptyCondition.Wait();
        }
        ASSERT_EQ(static_cast<int>(fds[1]), mEventVector[0].data.fd);
    }

    monitor.RemoveFD(fds[1]);
    monitor.Shutdown();
}

TEST_F(EPollMonitorOnBoxTest, CanAddRemoveFDs)
{
    EPollMonitor monitor("test");
    monitor.Start();

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fda(fds[0]);
    Forte::AutoFD fdb(fds[1]);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
    ev.data.ptr = NULL;
    ev.data.fd = fds[1];

    monitor.AddFD(
        fds[1],
        ev,
        boost::bind(&EPollMonitorOnBoxTest::handleEPollEvent, this, _1));
    monitor.RemoveFD(fds[1]);

    monitor.AddFD(
        fds[1],
        ev,
        boost::bind(&EPollMonitorOnBoxTest::handleEPollEvent, this, _1));

    if (send(fds[0], "after1adddel", 12, 0) == -1)
    {
        hlog(HLOG_WARN, "test data didn't send");
    }

    {
        AutoUnlockMutex lock(mEventVectorMutex);
        while (mEventVector.empty())
        {
            mEventVectorNotEmptyCondition.Wait();
        }
        ASSERT_EQ(static_cast<int>(fds[1]), mEventVector[0].data.fd);
    }

    monitor.RemoveFD(fds[1]);
    monitor.Shutdown();
}

TEST_F(EPollMonitorOnBoxTest, DataSentWhileEPollDown)
{
    EPollMonitor monitor("test");
    monitor.Start();

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fda(fds[0]);
    Forte::AutoFD fdb(fds[1]);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
    ev.data.ptr = NULL;
    ev.data.fd = fds[1];

    monitor.AddFD(
        fds[1],
        ev,
        boost::bind(&EPollMonitorOnBoxTest::handleEPollEvent, this, _1));
    monitor.RemoveFD(fds[1]);

    if (send(fds[0], "afterdel", 8, 0) != 8)
    {
        hlog(HLOG_WARN, "test data didn't send");
    }

    monitor.AddFD(
        fds[1],
        ev,
        boost::bind(&EPollMonitorOnBoxTest::handleEPollEvent, this, _1));

    {
        AutoUnlockMutex lock(mEventVectorMutex);
        while (mEventVector.empty())
        {
            char c;
            if (recv(ev.data.fd, &c, 1, MSG_PEEK|MSG_DONTWAIT) > 0)
            {
                hlog(HLOG_INFO, "data available in recv buf, no cb yet");
            }

            mEventVectorNotEmptyCondition.TimedWait(1);
        }
        ASSERT_EQ(static_cast<int>(fds[1]), mEventVector[0].data.fd);
    }
    monitor.RemoveFD(fds[1]);

    monitor.Shutdown();
}

TEST_F(EPollMonitorOnBoxTest, RecvEventET)
{
    EPollMonitor monitor("test");
    monitor.Start();

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fda(fds[0]);
    Forte::AutoFD fdb(fds[1]);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    ev.data.ptr = NULL;
    ev.data.fd = fds[1];

    monitor.AddFD(
        fds[1],
        ev,
        boost::bind(&EPollMonitorOnBoxTest::handleEPollEvent, this, _1));


    if (send(fds[0], "recv", 4, 0) == -1)
    {
        hlog(HLOG_WARN, "test data didn't send");
    }

    {
        AutoUnlockMutex lock(mEventVectorMutex);
        while (mEventVector.empty())
        {
            mEventVectorNotEmptyCondition.Wait();
        }
        ASSERT_EQ(static_cast<int>(fds[1]), mEventVector[0].data.fd);
    }

    monitor.RemoveFD(fds[1]);
    monitor.Shutdown();
}

TEST_F(EPollMonitorOnBoxTest, CanAddRemoveFDsET)
{
    EPollMonitor monitor("test");
    monitor.Start();

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fda(fds[0]);
    Forte::AutoFD fdb(fds[1]);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    ev.data.ptr = NULL;
    ev.data.fd = fds[1];

    monitor.AddFD(
        fds[1],
        ev,
        boost::bind(&EPollMonitorOnBoxTest::handleEPollEvent, this, _1));
    monitor.RemoveFD(fds[1]);

    monitor.AddFD(
        fds[1],
        ev,
        boost::bind(&EPollMonitorOnBoxTest::handleEPollEvent, this, _1));

    if (send(fds[0], "after1adddel", 12, 0) == -1)
    {
        hlog(HLOG_WARN, "test data didn't send");
    }

    {
        AutoUnlockMutex lock(mEventVectorMutex);
        while (mEventVector.empty())
        {
            mEventVectorNotEmptyCondition.Wait();
        }
        ASSERT_EQ(static_cast<int>(fds[1]), mEventVector[0].data.fd);
    }

    monitor.RemoveFD(fds[1]);
    monitor.Shutdown();
}

TEST_F(EPollMonitorOnBoxTest, DataSentWhileEPollDownET)
{
    EPollMonitor monitor("test");
    monitor.Start();

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fda(fds[0]);
    Forte::AutoFD fdb(fds[1]);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    ev.data.ptr = NULL;
    ev.data.fd = fds[1];

    monitor.AddFD(
        fds[1],
        ev,
        boost::bind(&EPollMonitorOnBoxTest::handleEPollEvent, this, _1));
    monitor.RemoveFD(fds[1]);

    if (send(fds[0], "afterdel", 8, 0) != 8)
    {
        hlog(HLOG_WARN, "test data didn't send");
    }

    monitor.AddFD(
        fds[1],
        ev,
        boost::bind(&EPollMonitorOnBoxTest::handleEPollEvent, this, _1));

    {
        AutoUnlockMutex lock(mEventVectorMutex);
        while (mEventVector.empty())
        {
            char c;
            if (recv(ev.data.fd, &c, 1, MSG_PEEK|MSG_DONTWAIT) > 0)
            {
                hlog(HLOG_INFO, "data available in recv buf, no cb yet");
            }

            mEventVectorNotEmptyCondition.TimedWait(1);
        }
        ASSERT_EQ(static_cast<int>(fds[1]), mEventVector[0].data.fd);
    }
    monitor.RemoveFD(fds[1]);

    monitor.Shutdown();
}

