#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"

#include "AutoFD.h"
#include "FunctionThread.h"
#include "AutoMutex.h"
#include "SocketUtil.h"

#include <boost/bind.hpp>
#include <boost/shared_array.hpp>

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

TEST_F(SocketUtilOnBoxTest, blocksWhenTCPBufferFull)
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

    int sendBufferSize = 0;
    int recvBufferSize = 0;
    socklen_t bufferSizeLength = sizeof(sendBufferSize);
    ASSERT_EQ(0, getsockopt(sock,
                            SOL_SOCKET,
                            SO_SNDBUF,
                            &sendBufferSize,
                            &bufferSizeLength));
    ASSERT_EQ(0, getsockopt(sock,
                            SOL_SOCKET,
                            SO_RCVBUF,
                            &recvBufferSize,
                            &bufferSizeLength));
    //seems like this should block:
    //int bufferSize = sendBufferSize + 1;
    //or at least this:
    //int bufferSize = (sendBufferSize + recvBufferSize) + 1;
    // or this?
    //int bufferSize = (sendBufferSize + recvBufferSize) + 5;
    //this does
    int bufferSize = (sendBufferSize + recvBufferSize) * 2;
    boost::shared_array<char> buffer(new char[bufferSize]);
    setUserTimeout(5);
    send(sock, buffer.get(), bufferSize, MSG_NOSIGNAL);
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
