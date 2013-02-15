#include <sys/epoll.h>

#include "gtest/gtest.h"

#include "FTrace.h"
#include "LogManager.h"

#include "PDUPeerEndpointFactory.h"
#include "PDUPeerInProcessEndpoint.h"
#include "PDUPeerFileDescriptorEndpoint.h"

#include "GMockPDUPeerEndpoint.h"
#include "GMockDispatcher.h"

using namespace std;
using namespace boost;
using namespace Forte;

using ::testing::_;

LogManager logManager;

struct PDU_Test
{
    int a;
    int b;
    int c;
    char d[128];
} __attribute__((__packed__));

shared_array<char> makeTestPDU(Forte::PDU &pdu, size_t &len)
{
    PDUHeader header;
    header.version = Forte::PDU::PDU_VERSION;
    header.opcode = 1;
    header.payloadSize = sizeof(PDU_Test);
    pdu.SetHeader(header);
    pdu.SetPayload(sizeof(PDU_Test));

    PDU_Test *testPDU = pdu.GetPayload<PDU_Test>();
    testPDU->a = 1;
    testPDU->b = 2;
    testPDU->c = 3;
    snprintf(testPDU->d, sizeof(testPDU->d), "111.222.333.444");
    len = sizeof(Forte::PDUHeader) + header.payloadSize;

    // Return the full data buffer
    return PDU::CreateSendBuffer(pdu);
}
// -----------------------------------------------------------------------------

class PDUPeerFileDescriptorEndpointUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
        hlog(HLOG_DEBUG, "Starting test...");
    }

    static void TearDownTestCase() {
    }

    void SetUp() {
    }

    void TearDown() {
    }
};

TEST_F(PDUPeerFileDescriptorEndpointUnitTest, CanConstructWithNoFD)
{
    FTRACE;
    PDUPeerFileDescriptorEndpoint e(-1);
}

TEST_F(PDUPeerFileDescriptorEndpointUnitTest, IsPassedDataViaDataIn)
{
    FTRACE;

    Forte::PDU pdu;
    size_t len;
    shared_array<char> buf = makeTestPDU(pdu, len);

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fd1(fds[0]);
    Forte::AutoFD fd2(fds[1]);

    Forte::PDUPeerFileDescriptorEndpoint peer(fd1);
    send(fd2, buf.get(), len, 0);
    struct epoll_event event;
    event.events = EPOLLIN;
    peer.HandleEPollEvent(event);


    Forte::PDU out;
    bool received = peer.RecvPDU(out);
    ASSERT_EQ(received, true);

    ASSERT_EQ(pdu == out, true);
}


TEST_F(PDUPeerFileDescriptorEndpointUnitTest, KeepsIncomingPDUsInAQueue)
{
    FTRACE;

    Forte::PDU pdu;
    size_t len;
    shared_array<char> buf = makeTestPDU(pdu, len);

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fd1(fds[0]);
    Forte::AutoFD fd2(fds[1]);

    Forte::PDUPeerFileDescriptorEndpoint peer(fd1);

    for (int i = 0; i < 3; ++i)
    {
        send(fd2, buf.get(), len, 0);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    peer.HandleEPollEvent(event);

    Forte::PDU out;
    for (int i = 0; i < 3; ++i)
    {
        bool received = peer.RecvPDU(out);
        ASSERT_EQ(received, true);

        ASSERT_EQ(pdu == out, true);
    }
}

TEST_F(PDUPeerFileDescriptorEndpointUnitTest, HandlesPDUsInternally)
{
    FTRACE;

    try
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        Forte::AutoFD fd1(fds[0]);
        Forte::AutoFD fd2(fds[1]);

        AutoFD epollFD;
        epollFD = epoll_create(4);
        Forte::PDUPeerFileDescriptorEndpoint endpoint1(fd1);
        Forte::PDUPeerFileDescriptorEndpoint endpoint2(fd2);

        endpoint1.SetEPollFD(epollFD);
        endpoint2.SetEPollFD(epollFD);

        PDU pdu, rpdu;
        size_t len;
        makeTestPDU(pdu, len);

        endpoint1.SendPDU(pdu);

        // poll for events
        int fdReadyCount = 0;
        struct epoll_event events[32];
        memset(events, 0, 32*sizeof(struct epoll_event));
        fdReadyCount = epoll_wait(epollFD, events, 32, 1000);
        EXPECT_EQ(1, fdReadyCount);
        ASSERT_EQ((int) fd2, events[0].data.fd);
        ASSERT_EQ(endpoint2.GetFD(), events[0].data.fd);

        endpoint2.HandleEPollEvent(events[0]);

        EXPECT_EQ(true, endpoint2.IsPDUReady());
        EXPECT_EQ(true, endpoint2.RecvPDU(rpdu));
        EXPECT_EQ(true, pdu == rpdu);
    }
    catch (Forte::Exception &e)
    {
        hlog(HLOG_ERR, "caught exception: %s\n", e.what());
        FAIL();
    }
}

class AsyncDataLoaderThread : public Forte::Thread
{
public:
    AsyncDataLoaderThread(Forte::PDUPeerFileDescriptorEndpoint& peer, int fd)
        : mPeer(peer),
          mFD(fd),
          mReceiveCount(0)
        {
            initialized();
        }

    ~AsyncDataLoaderThread() {
        deleting();
    }

    int GetReceiveCount() {
        AutoUnlockMutex lock(mReceiveCountMutex);
        return mReceiveCount;
    }

protected:
    void* run() {
        Forte::PDU pdu;
        size_t len;
        shared_array<char> buf = makeTestPDU(pdu, len);

        for (int i = 0; i < 100; ++i)
        {
            send(mFD, buf.get(), len, 0);
            struct epoll_event event;
            event.events = EPOLLIN;
            mPeer.HandleEPollEvent(event);
            {
                AutoUnlockMutex lock(mReceiveCountMutex);
                mReceiveCount++;
            }
        }
        return NULL;
    }

protected:
    Forte::PDUPeerFileDescriptorEndpoint& mPeer;
    int mFD;
    Forte::Mutex mReceiveCountMutex;
    int mReceiveCount;
};

TEST_F(PDUPeerFileDescriptorEndpointUnitTest,
       BlocksOnBufferFullUntilRecvPDUIsCalled)
{
    FTRACE;

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fd1(fds[0]);
    Forte::AutoFD fd2(fds[1]);

    Forte::PDUPeerFileDescriptorEndpoint peer(fd1, 512, 768);
    AsyncDataLoaderThread t(peer, fd2);

    sleep(1);
    ASSERT_EQ(t.GetReceiveCount(), 3);

    PDU pdu;
    while (t.GetReceiveCount() != 100)
    {
        while (peer.RecvPDU(pdu))
        {
        }
        usleep(10000);
    }
    ASSERT_EQ(100, t.GetReceiveCount());

    t.Shutdown();
    t.WaitForShutdown();
}

TEST_F(PDUPeerFileDescriptorEndpointUnitTest, ThrowESendFailedOnBrokenPipe)
{
    FTRACE;

    try
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        Forte::AutoFD fd1(fds[0]);
        Forte::AutoFD fd2(fds[1]);

        AutoFD epollFD;
        epollFD = epoll_create(4);
        Forte::PDUPeerFileDescriptorEndpoint endpoint1(fd1);
        Forte::PDUPeerFileDescriptorEndpoint endpoint2(fd2);

        endpoint1.SetEPollFD(epollFD);
        endpoint2.SetEPollFD(epollFD);

        PDU pdu, rpdu;
        size_t len;
        makeTestPDU(pdu, len);

        endpoint1.SendPDU(pdu);

        // poll for events
        int fdReadyCount = 0;
        struct epoll_event events[32];
        memset(events, 0, 32*sizeof(struct epoll_event));
        fdReadyCount = epoll_wait(epollFD, events, 32, 1000);
        EXPECT_EQ(1, fdReadyCount);
        ASSERT_EQ((int) fd2, events[0].data.fd);
        ASSERT_EQ(endpoint2.GetFD(), events[0].data.fd);

        endpoint2.HandleEPollEvent(events[0]);

        EXPECT_EQ(true, endpoint2.IsPDUReady());
        EXPECT_EQ(true, endpoint2.RecvPDU(rpdu));
        EXPECT_EQ(true, pdu == rpdu);

        close(fds[1]);

        ASSERT_THROW(endpoint1.SendPDU(pdu), EPeerSendFailed);

    }
    catch (Forte::Exception &e)
    {
        hlog(HLOG_ERR, "caught exception: %s\n", e.what());
        FAIL();
    }
}
