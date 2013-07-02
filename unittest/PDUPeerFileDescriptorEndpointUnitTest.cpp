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

class PDUPeerFileDescriptorEndpointUnitTest : public ::testing::Test
{
public:
    PDUPeerFileDescriptorEndpointUnitTest()
        : mEventReceivedCondition(mEventMutex) {}

    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
        hlog(HLOG_DEBUG, "Starting test...");
    }

    static void TearDownTestCase() {
        logManager.EndLogging();
    }

    void SetUp() {
        mReceiveEventCount = 0;
        mSendErrorEventCount = 0;
        mConnectedEventCount = 0;
        mDisconnectedEventCount = 0;
    }

    void TearDown() {
        if (mConnectedEventCount != mDisconnectedEventCount)
        {
            hlogstream(
                HLOG_WARN,
                "connect event count (" << mConnectedEventCount << ") "
                "!= disconnect event count (" << mDisconnectedEventCount << ")");
        }
        else
        {
            hlogstream(
                HLOG_INFO,
                "connect/disconnect event count equal ("
                << mConnectedEventCount << ") ");
        }

    }

    void setupDefaultFDPair() {
        FTRACE;
        mMonitor.reset(new EPollMonitor);
        mPDUQueue1.reset(new PDUQueue);
        mPDUQueue2.reset(new PDUQueue);

        mMonitor->Start();

        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        mFD1 = fds[0];
        mFD2 = fds[1];

        mEndpoint1.reset(new PDUPeerFileDescriptorEndpoint(mPDUQueue1, mMonitor));
        mEndpoint2.reset(new PDUPeerFileDescriptorEndpoint(mPDUQueue2, mMonitor));

        mEndpoint1->SetEventCallback(
            boost::bind(
                &PDUPeerFileDescriptorEndpointUnitTest::EventCallback, this, _1));

        mEndpoint2->SetEventCallback(
            boost::bind(
                &PDUPeerFileDescriptorEndpointUnitTest::EventCallback, this, _1));

        mEndpoint1->SetFD(mFD1);
        mEndpoint2->SetFD(mFD2);

        mEndpoint1->Start();
        mEndpoint2->Start();
    }

    void teardownDefaultFDPair() {
        mEndpoint1->SetFD(-1);
        mEndpoint2->SetFD(-1);
        mEndpoint2->SetEventCallback(NULL);
        mEndpoint1->Shutdown();
        mEndpoint2->Shutdown();
        mMonitor->Shutdown();
    }

    void EventCallback(PDUPeerEventPtr event) {
        Forte::AutoUnlockMutex lock(mEventMutex);
        switch (event->mEventType)
        {
        case PDUPeerReceivedPDUEvent:
            hlog(HLOG_INFO, "Recvd PDU Event");
            ++mReceiveEventCount;
            break;

        case PDUPeerSendErrorEvent:
            hlog(HLOG_INFO, "Recvd Send Error Event");
            ++mSendErrorEventCount;
            break;

        case PDUPeerConnectedEvent:
            hlog(HLOG_INFO, "Recvd Connect Event");
            ++mConnectedEventCount;
            break;

        case PDUPeerDisconnectedEvent:
            hlog(HLOG_INFO, "Recvd Disconnect Event");
            ++mDisconnectedEventCount;
            break;

        default:
            break;
        }

        mEventReceivedCondition.Signal();
    }

    Forte::Mutex mEventMutex;
    Forte::ThreadCondition mEventReceivedCondition;
    int mReceiveEventCount;
    int mSendErrorEventCount;
    int mConnectedEventCount;
    int mDisconnectedEventCount;

    boost::shared_ptr<EPollMonitor> mMonitor;
    Forte::AutoFD mFD1;
    Forte::AutoFD mFD2;
    boost::shared_ptr<PDUQueue> mPDUQueue1;
    boost::shared_ptr<PDUQueue> mPDUQueue2;
    boost::shared_ptr<PDUPeerFileDescriptorEndpoint> mEndpoint1;
    boost::shared_ptr<PDUPeerFileDescriptorEndpoint> mEndpoint2;
};

struct TestPayload
{
    TestPayload()
        : a(0), b(0), c(0)
        {
            memset(d, 0, 128);
        }
    int a;
    int b;
    int c;
    char d[128];
} __attribute__((__packed__));

boost::shared_ptr<PDU> makeTestPDU()
{
    PDUPtr pdu;

    PDUHeader header;
    header.version = Forte::PDU::PDU_VERSION;
    header.opcode = 1;
    header.payloadSize = sizeof(TestPayload);

    boost::shared_array<char> optionalDataPayload(new char[4096]);
    memset(optionalDataPayload.get(), 'z', 4096);
    boost::shared_ptr<PDUOptionalData> o(
        new PDUOptionalData(4096, 0, optionalDataPayload.get()));

    TestPayload payload;
    payload.a = 1;
    payload.b = 2;
    payload.c = 3;
    snprintf(payload.d, sizeof(payload.d), "111.222.333.444");

    pdu.reset(new PDU(header.opcode, sizeof(payload), &payload));
    pdu->SetOptionalData(o);

    return pdu;
}

size_t testPDUSize(const boost::shared_ptr<PDU>& pdu)
{
    return sizeof(pdu->GetHeader())
        + pdu->GetPayloadSize()
        + pdu->GetOptionalDataSize();
}

TEST_F(PDUPeerFileDescriptorEndpointUnitTest, ConstructDelete)
{
    FTRACE;
    boost::shared_ptr<PDUQueue> pduQueue(new PDUQueue);
    boost::shared_ptr<EPollMonitor> monitor(new EPollMonitor);
    PDUPeerFileDescriptorEndpoint e(pduQueue, monitor);
}

TEST_F(PDUPeerFileDescriptorEndpointUnitTest, SetFDTriggersConnectedCallback)
{
    FTRACE;
    boost::shared_ptr<EPollMonitor> monitor(new EPollMonitor);
    monitor->Start();

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fd1(fds[0]);
    Forte::AutoFD fd2(fds[1]);

    boost::shared_ptr<PDUQueue> pduQueue1(new PDUQueue);
    boost::shared_ptr<PDUPeerFileDescriptorEndpoint> e1(
        new PDUPeerFileDescriptorEndpoint(pduQueue1, monitor));

    e1->SetEventCallback(
        boost::bind(
            &PDUPeerFileDescriptorEndpointUnitTest::EventCallback, this, _1));

    e1->Start();

    e1->SetFD(fds[0]);

    {
        Forte::AutoUnlockMutex lock(mEventMutex);
        while (mConnectedEventCount < 1)
        {
            mEventReceivedCondition.Wait();
        }
    }

    EXPECT_EQ(1, mConnectedEventCount);



    EXPECT_EQ(1, mConnectedEventCount);

    e1->Shutdown();

    monitor->Shutdown();
}

TEST_F(PDUPeerFileDescriptorEndpointUnitTest, SendsAndRecvsViaEPollAndPDUQueue)
{
    FTRACE;
    boost::shared_ptr<EPollMonitor> monitor(new EPollMonitor);
    monitor->Start();

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fd1(fds[0]);
    Forte::AutoFD fd2(fds[1]);

    boost::shared_ptr<PDUQueue> pduQueue1(new PDUQueue);
    boost::shared_ptr<PDUPeerFileDescriptorEndpoint> e1(
        new PDUPeerFileDescriptorEndpoint(pduQueue1, monitor));
    e1->SetFD(fds[0]);

    boost::shared_ptr<PDUQueue> pduQueue2(new PDUQueue);
    boost::shared_ptr<PDUPeerFileDescriptorEndpoint> e2(
        new PDUPeerFileDescriptorEndpoint(pduQueue2, monitor));
    e2->SetFD(fds[1]);

    e1->Start();
    e2->Start();

    Forte::PDUPtr pdu = makeTestPDU();
    pduQueue1->EnqueuePDU(pdu);

    Forte::PDU out;
    while (!e2->IsPDUReady()) { usleep(10000); }

    bool received = e2->RecvPDU(out);
    EXPECT_EQ(received, true);
    EXPECT_EQ(*pdu, out);

    e1->Shutdown();
    e2->Shutdown();

    monitor->Shutdown();
}

TEST_F(PDUPeerFileDescriptorEndpointUnitTest, QueuesPDUsForSending)
{
    FTRACE;
    setupDefaultFDPair();

    Forte::PDUPtr pdu = makeTestPDU();
    mPDUQueue1->EnqueuePDU(pdu);
    mPDUQueue1->EnqueuePDU(pdu);
    mPDUQueue1->EnqueuePDU(pdu);

    Forte::PDU out;
    for (int i = 0; i < 3; ++i)
    {
        while (!mEndpoint2->IsPDUReady()) { usleep(10000); }
        bool received = mEndpoint2->RecvPDU(out);
        ASSERT_TRUE(received);
        ASSERT_EQ(*pdu, out);
    }

    teardownDefaultFDPair();
}

TEST_F(PDUPeerFileDescriptorEndpointUnitTest,
       FiresDisconnect_OnCloseFD_EPollErr)
{
    FTRACE;

    setupDefaultFDPair();

    {
        Forte::AutoUnlockMutex lock(mEventMutex);
        while (mConnectedEventCount < 2)
        {
            mEventReceivedCondition.Wait();
        }
    }
    EXPECT_EQ(2, mConnectedEventCount);

    struct epoll_event ev;
    ev.events = EPOLLERR;
    mEndpoint1->HandleEPollEvent(ev);

    {
        Forte::AutoUnlockMutex lock(mEventMutex);
        while (mDisconnectedEventCount < 2)
        {
            mEventReceivedCondition.Wait();
        }
    }
    EXPECT_EQ(2, mDisconnectedEventCount);

    teardownDefaultFDPair();
}

/*TODO:
TEST_F(PDUPeerSetImplOnBoxTest, CanReceiveDataWhenSendQueueIsFull)
{
}
*/
