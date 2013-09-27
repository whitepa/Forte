#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"

#include "AutoFD.h"
#include "FunctionThread.h"
#include "AutoMutex.h"
#include "AutoDoUndo.h"
#include "SocketUtil.h"
#include "SystemCallUtil.h"

#include <boost/bind.hpp>
#include <boost/shared_array.hpp>
#include <poll.h>
#include <netinet/tcp.h>

using namespace std;
using namespace boost;
using namespace Forte;
using ::testing::UnitTest;

LogManager logManager;

class SocketUtilOnBoxTest : public ::testing::Test
{
public:
    SocketUtilOnBoxTest()
        : mListening(false),
          mListeningMutex(),
          mListeningCondition(mListeningMutex)
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
            << UnitTest::GetInstance()->current_test_info()->test_case_name());
    }

    void TearDown() {
        hlogstream(
            HLOG_INFO, "ending test "
            << UnitTest::GetInstance()->current_test_info()->test_case_name());
    }

    void listenAndExitThreadRun() {
        AutoFD sock;
        if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        {
            hlog(HLOG_ERR, "couldn't create socket");
            return;
        }
        bindToAddress(sock, SocketAddress("127.0.0.1", 21472));
        if (listen(sock, 1) == -1)
        {
            hlog(HLOG_ERR, "couldn't listen on socket");
        }

        {
            AutoUnlockMutex lock(mListeningMutex);
            mListening = true;
            mListeningCondition.Signal();
        }

        Thread::MyThread()->InterruptibleSleep(Timespec::FromSeconds(1000));
    }

    bool mListening;
    Forte::Mutex mListeningMutex;
    Forte::ThreadCondition mListeningCondition;

};

TEST_F(SocketUtilOnBoxTest, connectFailsImmediatelyIfNoListen)
{
    FTRACE;

    SocketAddress sa("127.0.0.1", 21471); //hopefully unused

    TimerClock c;
    c.Start();
    AutoFD sock(createInetStreamSocket());
    EXPECT_THROW(connectToAddress(sock, sa), ECouldNotConnect);
    c.Stop();
    EXPECT_LE(c.GetTime().AsSeconds(), 1);
}

TEST_F(SocketUtilOnBoxTest, connectSucceedBasedOnAcceptingSocketInListenState)
{
    FTRACE;
    Forte::FunctionThread t(
        Forte::FunctionThread::AutoInit(),
        boost::bind(&SocketUtilOnBoxTest::listenAndExitThreadRun, this),
        "blking-acpt-thr");

    {
        AutoUnlockMutex lock(mListeningMutex);
        while (!mListening)
        {
            mListeningCondition.Wait();
        }
    }

    SocketAddress sa("127.0.0.1", 21472);

    TimerClock c;
    c.Start();
    AutoFD sock(createInetStreamSocket());
    connectToAddress(sock, sa);
    c.Stop();
    EXPECT_LE(c.GetTime().AsSeconds(), 1);
}

TEST_F(SocketUtilOnBoxTest, CanTimeoutBlockedConnectionWithKeepAlive)
{
    FTRACE;
    Forte::FunctionThread t(
        Forte::FunctionThread::AutoInit(),
        boost::bind(&SocketUtilOnBoxTest::listenAndExitThreadRun, this),
        "blking-acpt-thr");

    {
        AutoUnlockMutex lock(mListeningMutex);
        while (!mListening)
        {
            mListeningCondition.Wait();
        }
    }

    SocketAddress sa("127.0.0.1", 21472);
    AutoFD sock(createInetStreamSocket());

    connectToAddress(sock, sa);
    const int keepAliveEnabled (1);
    const int keepAliveCount (1);
    const int keepAliveIntervalSecs (2);
    const int keepAliveIdleSecs (1);

    // will start keep alive packets after keepAliveIdle seconds,
    // sending keepAliveCount probes. Interval between probes is
    // keepAliveIntervalSecs. So a idle down connection should
    // timeout in
    // keepAliveIdleSecs + (keepAliveIntervalSecs * keepAliveCount) =
    // 1 + (2 * 1) = 3 seconds;
    setTCPKeepAlive(
        sock,
        keepAliveEnabled,
        keepAliveCount,
        keepAliveIntervalSecs,
        keepAliveIdleSecs);

    // lets expire the timer in 4 seconds
    DeadlineClock deadline;
    deadline.ExpiresInSeconds(4);

    system("/sbin/iptables -I INPUT -m tcp -p tcp --dport 21472 -j DROP");
    AutoDoUndo autoDisableFilter (
        boost::bind(&system,
                    "/sbin/iptables -D INPUT -m tcp -p tcp --dport 21472 -j DROP"));

    // let's poll on FD to get error
    bool pollErrorOccured = false;
    struct pollfd pollFDs[1];
    pollFDs[0].events = POLLIN | POLLERR;
    const int pollFDCount(1);
    pollFDs[0].fd = sock;

    int rc;
    rc = poll(pollFDs,
              pollFDCount,
              30000);
    if (rc < 0)
    {
        hlog(HLOG_ERR, "poll(): %s",
             SystemCallUtil::GetErrorDescription(errno).c_str());
    }
    else if (rc > 0)
    {
        if ((pollFDs[0].revents & POLLERR))
        {
            hlog(HLOG_INFO, "fd errored out");
            pollErrorOccured = true;
        }
    }
    else
    {
        hlog(HLOG_ERR, "Poll timed out");
    }

    Forte::Timespec remaining;
    deadline.GetRemaining(remaining);
    hlog(HLOG_INFO, "Deadline has %lld remaining", remaining.AsMillisec());

    ASSERT_TRUE(pollErrorOccured);
    ASSERT_FALSE(deadline.Expired());
}

TEST_F(SocketUtilOnBoxTest, CanTimeoutBlockedSendWithSetUserTimeout)
{
    FTRACE;
    Forte::FunctionThread t(
        Forte::FunctionThread::AutoInit(),
        boost::bind(&SocketUtilOnBoxTest::listenAndExitThreadRun, this),
        "blking-acpt-thr");

    {
        AutoUnlockMutex lock(mListeningMutex);
        while (!mListening)
        {
            mListeningCondition.Wait();
        }
    }

    SocketAddress sa("127.0.0.1", 21472);
    AutoFD sock(createInetStreamSocket());

    connectToAddress(sock, sa);
    const int userTimeoutMilliSec (1000);
    setTCPUserTimeout(sock, userTimeoutMilliSec);

    system("/sbin/iptables -I INPUT -m tcp -p tcp --dport 21472 -j DROP");
    AutoDoUndo autoDisableFilter (
        boost::bind(&system,
                    "/sbin/iptables -D INPUT -m tcp -p tcp --dport 21472 -j DROP"));

    bool pollErrorOccured = false;
    int bufferSize = 2;
    hlog(HLOG_INFO, "Buffer size to send %d", bufferSize);
    boost::shared_array<char> buffer(new char[bufferSize]);
    int err;
    // the send should succeed at putting the data in the
    // kernel send queue
    if ((err = send(sock, buffer.get(), bufferSize, MSG_NOSIGNAL)) < 0)
    {
        hlog(HLOG_INFO, "Send failed %d, %s",
             err, Forte::SystemCallUtil::GetErrorDescription(errno).c_str());
    }
    else
    {
        // let set deadline to 2 seconds
        DeadlineClock deadline;
        deadline.ExpiresInSeconds(2);

        // after the send, in userTimeoutMilliSecs (1000),
        // connection should timeout, which should happen
        // in this poll.
        struct pollfd pollFDs[1];
        pollFDs[0].events = POLLIN | POLLERR;
        const int pollFDCount(1);
        pollFDs[0].fd = sock;

        int rc;
        rc = poll(pollFDs,
                  pollFDCount,
                  30000);
        if (rc < 0)
        {
            hlog(HLOG_ERR, "poll(): %s",
                 SystemCallUtil::GetErrorDescription(errno).c_str());
        }
        else if (rc > 0)
        {
            if ((pollFDs[0].revents & POLLERR))
            {
                hlog(HLOG_INFO, "fd errored out");
                pollErrorOccured = true;
            }
        }
        else
        {
            hlog(HLOG_ERR, "Poll timed out");
        }

        Forte::Timespec remaining;
        deadline.GetRemaining(remaining);
        hlog(HLOG_INFO, "Deadline has %lld remaining", remaining.AsMillisec());

        ASSERT_FALSE(deadline.Expired());
    }

    ASSERT_TRUE(pollErrorOccured);
}

TEST_F(SocketUtilOnBoxTest, connectCanConnectToAcceptor)
{
    FTRACE;

    /*Forte::FunctionThread t(
        Forte::FunctionThread::AutoInit(),
        boost::bind(&SocketUtilOnBoxTest::listenAndExitThreadRun, this),
        "blking-acpt-thr");

    SocketAddress sa("127.0.0.1", 21472);

    TimerClock c;
    c.Start();
    EXPECT_THROW(connectToAddress(sa), ECouldNotConnect);
    c.Stop();

    EXPECT_LE(c.GetTime().AsSeconds(), 1);*/
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
TEST_F(SocketUtilOnBoxTest, NonBlockingConnectCanTimeout)
{
    // don't start the listener

    struct sockaddr_in address;
    SocketAddress sa("1.2.0.1", 21472); // some invalid address
    AutoFD sock(createInetStreamSocket());

    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(sa.first.c_str());
    address.sin_port = htons(sa.second);

    EXPECT_THROW(
        connectNonBlocking(
            sock,
            reinterpret_cast<struct sockaddr *>(&address),
            sizeof(struct sockaddr_in),
            Forte::Timespec::FromMillisec(500)),
        ECouldNotConnect);
}

TEST_F(SocketUtilOnBoxTest, NonBlockingConnect)
{
    Forte::FunctionThread t(
        Forte::FunctionThread::AutoInit(),
        boost::bind(&SocketUtilOnBoxTest::listenAndExitThreadRun, this),
        "blking-acpt-thr");

    {
        AutoUnlockMutex lock(mListeningMutex);
        while (!mListening)
        {
            mListeningCondition.Wait();
        }
    }

    struct sockaddr_in address;
    SocketAddress sa("127.0.0.1", 21472); // some invalid address
    AutoFD sock(createInetStreamSocket());

    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(sa.first.c_str());
    address.sin_port = htons(sa.second);

    EXPECT_NO_THROW(
        connectNonBlocking(
            sock,
            reinterpret_cast<struct sockaddr *>(&address),
            sizeof(struct sockaddr_in),
            Forte::Timespec::FromMillisec(500)));
}
