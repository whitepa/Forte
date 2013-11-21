#include <sys/epoll.h>

#include "gtest/gtest.h"

#include "FTrace.h"
#include "LogManager.h"

#include "PDUPeerEndpointFactory.h"
#include "PDUPeerEndpointInProcess.h"
#include "PDUPeerEndpointFD.h"

#include "GMockPDUPeerEndpoint.h"
#include "GMockDispatcher.h"

using namespace std;
using namespace boost;
using namespace Forte;

using ::testing::_;

LogManager logManager;

class PDUPeerEndpointFDUnitTest : public ::testing::TestWithParam<int>
{
public:
    PDUPeerEndpointFDUnitTest()
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

        mEndpoint1.reset(new PDUPeerEndpointFD(mPDUQueue1, mMonitor));
        mEndpoint2.reset(new PDUPeerEndpointFD(mPDUQueue2, mMonitor));

        mEndpoint1->SetEventCallback(
            boost::bind(
                &PDUPeerEndpointFDUnitTest::EventCallback, this, _1));

        mEndpoint2->SetEventCallback(
            boost::bind(
                &PDUPeerEndpointFDUnitTest::EventCallback, this, _1));

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
            //hlog(HLOG_INFO, "Recvd PDU Event");
            ++mReceiveEventCount;
            break;

        case PDUPeerSendErrorEvent:
            //hlog(HLOG_INFO, "Recvd Send Error Event");
            ++mSendErrorEventCount;
            break;

        case PDUPeerConnectedEvent:
            //hlog(HLOG_INFO, "Recvd Connect Event");
            ++mConnectedEventCount;
            break;

        case PDUPeerDisconnectedEvent:
            //hlog(HLOG_INFO, "Recvd Disconnect Event");
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
    boost::shared_ptr<PDUPeerEndpointFD> mEndpoint1;
    boost::shared_ptr<PDUPeerEndpointFD> mEndpoint2;
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

boost::shared_ptr<PDU> makeTestPDU(size_t optionalDataSize=4096)
{
    PDUPtr pdu;

    PDUHeader header;
    header.version = Forte::PDU::PDU_VERSION;
    header.opcode = 1;
    header.payloadSize = sizeof(TestPayload);

    boost::shared_array<char> optionalDataPayload(new char[optionalDataSize]);
    memset(optionalDataPayload.get(), 'z', optionalDataSize);
    boost::shared_ptr<PDUOptionalData> o(
        new PDUOptionalData(optionalDataSize, 0, optionalDataPayload.get()));

    TestPayload payload;
    payload.a = 1;
    payload.b = 2;
    payload.c = 3;
    snprintf(payload.d, sizeof(payload.d), "111.222.333.444");

    pdu.reset(new PDU(header.opcode, sizeof(payload), &payload));
    pdu->SetOptionalData(o);

    return pdu;
}

boost::shared_ptr<PDU> makeRandomPDU()
{
    PDUPtr pdu;

    size_t optionalDataSize = rand() % (65536 - 4096);

    PDUHeader header;
    header.version = Forte::PDU::PDU_VERSION;
    header.opcode = rand() % 127;
    header.payloadSize = sizeof(TestPayload);

    boost::shared_array<char> optionalDataPayload(new char[optionalDataSize]);
    for (unsigned int i=0; i<optionalDataSize; ++i)
    {
        optionalDataPayload.get()[i] = i;
    }
    boost::shared_ptr<PDUOptionalData> o(
        new PDUOptionalData(optionalDataSize,
                            10+(rand()%5),
                            optionalDataPayload.get()));

    TestPayload payload;
    payload.a = rand();
    payload.b = payload.a + 1;
    payload.c = payload.a + 2;
    memset(payload.d, 0, sizeof(payload.d));

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

size_t testPDUMaxSize()
{
    return DEFAULT_MAX_BUFFER_SIZE - sizeof(PDUHeader) - sizeof(TestPayload);
}

TEST_F(PDUPeerEndpointFDUnitTest, ConstructDelete)
{
    FTRACE;
    boost::shared_ptr<PDUQueue> pduQueue(new PDUQueue);
    boost::shared_ptr<EPollMonitor> monitor(new EPollMonitor);
    PDUPeerEndpointFD e(pduQueue, monitor);
}

TEST_F(PDUPeerEndpointFDUnitTest, SetFDTriggersConnectedCallback)
{
    FTRACE;
    boost::shared_ptr<EPollMonitor> monitor(new EPollMonitor);
    monitor->Start();

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fd1(fds[0]);
    Forte::AutoFD fd2(fds[1]);

    boost::shared_ptr<PDUQueue> pduQueue1(new PDUQueue);
    boost::shared_ptr<PDUPeerEndpointFD> e1(
        new PDUPeerEndpointFD(pduQueue1, monitor));

    e1->SetEventCallback(
        boost::bind(
            &PDUPeerEndpointFDUnitTest::EventCallback, this, _1));

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

    e1->Shutdown();

    monitor->Shutdown();
}

TEST_F(PDUPeerEndpointFDUnitTest, SendsAndRecvsViaEPollAndPDUQueue)
{
    FTRACE;
    boost::shared_ptr<EPollMonitor> monitor(new EPollMonitor);
    monitor->Start();

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    Forte::AutoFD fd1(fds[0]);
    Forte::AutoFD fd2(fds[1]);

    boost::shared_ptr<PDUQueue> pduQueue1(new PDUQueue);
    boost::shared_ptr<PDUPeerEndpointFD> e1(
        new PDUPeerEndpointFD(pduQueue1, monitor));
    e1->SetFD(fds[0]);

    boost::shared_ptr<PDUQueue> pduQueue2(new PDUQueue);
    boost::shared_ptr<PDUPeerEndpointFD> e2(
        new PDUPeerEndpointFD(pduQueue2, monitor));
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

TEST_F(PDUPeerEndpointFDUnitTest, QueuesPDUsForSending)
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

TEST_F(PDUPeerEndpointFDUnitTest, CanSendPDUUpToMaxPayload)
{
    FTRACE;
    setupDefaultFDPair();

    Forte::PDUPtr pdu = makeTestPDU(testPDUMaxSize());

    mPDUQueue1->EnqueuePDU(pdu);
    Forte::PDU out;
    while (!mEndpoint2->RecvPDU(out)) { usleep(10000); }
    ASSERT_EQ(*pdu, out);

    teardownDefaultFDPair();
}

TEST_F(PDUPeerEndpointFDUnitTest, PDUsCanHaveAnySizePayload)
{
    FTRACE;
    setupDefaultFDPair();

    for (int i = 0; i < 1024; ++i)
    {
        size_t thisPDUSize = rand() % testPDUMaxSize();
        //hlog(HLOG_INFO, "sending PDU with optional payload of %zu", thisPDUSize);
        Forte::PDUPtr pdu = makeTestPDU(thisPDUSize);
        mPDUQueue1->EnqueuePDU(pdu);

        Forte::PDU out;
        while (!mEndpoint2->RecvPDU(out)) { usleep(10000); }
        ASSERT_EQ(*pdu, out);
    }

    teardownDefaultFDPair();
}

TEST_F(PDUPeerEndpointFDUnitTest,
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

TEST_F(PDUPeerEndpointFDUnitTest, PDUPayloadVersionTest)
{
    FTRACE;
    setupDefaultFDPair();

    std::vector<Forte::PDUPtr> pdus;
    Forte::PDUPtr pdu = makeTestPDU();
    pdus.push_back(pdu);

    pdu = makeTestPDU();
    pdu->SetPayloadVersion(1);
    pdus.push_back(pdu);

    pdu = makeTestPDU();
    pdu->SetPayloadVersion(2);
    pdus.push_back(pdu);

    for (unsigned int i = 0; i < 3; ++i)
    {
        mPDUQueue1->EnqueuePDU(pdus.at(i));
    }

    Forte::PDU out;
    for (unsigned int i = 0; i < 3; ++i)
    {
        while (!mEndpoint2->IsPDUReady()) { usleep(10000); }
        bool received = mEndpoint2->RecvPDU(out);
        PDUPtr oldPDU = pdus.at(i);
        ASSERT_TRUE(received);
        ASSERT_TRUE(PDU::GetPayloadVersion(out.GetVersion()) == i);
    }

    teardownDefaultFDPair();
}

//TODO: mvoe this onbox. it's a baseline perf number for what
//PDUPeerEndpointFD needs to do
TEST_F(PDUPeerEndpointFDUnitTest,
       DISABLED_WillReadWritePDUsAsLongAsThereIsRoom)
{
    logManager.EndLogging(__FILE__ ".log");

    setupDefaultFDPair();

    TimerClock c;
    c.Start();
    const int iterations(1000);
    int totalIO(0);
    Forte::PDU out;
    Forte::PDUPtr pdu = makeTestPDU();

    for (int i=0; i<iterations; ++i)
    {
        while (mEndpoint2->RecvPDU(out))
        {
            ASSERT_EQ(*pdu, out);
            ++totalIO;
        }

        try
        {
            while (true)
            {
                mPDUQueue1->EnqueuePDU(pdu);
                ++totalIO;
            }
        }
        catch (EPDUQueueFull& e)
        {
        }
    }

    c.Stop();
    hlogstream(HLOG_INFO,
               iterations << " iterations took " << c.GetTime()
               << " and processed " << totalIO);

    //memmove impl baseline on laptop with ASSERT_EQ commented out
    //1000 iterations took 58.369622753 and processed 1003371

    teardownDefaultFDPair();

    logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
}

TEST_P(PDUPeerEndpointFDUnitTest, PDUsCanHaveConsistentSizePayloads)
{
    FTRACE;
    setupDefaultFDPair();

    Forte::PDUPtr pdu = makeTestPDU(GetParam());
    Forte::PDU out;

    //TODO: make an onbox test
    //for (int i = 0; i < 6400*3; ++i)
    for (int i = 0; i < 256; ++i)
    {
        try
        {
            mPDUQueue1->EnqueuePDU(pdu);
        }
        catch(...)
        {
            // in case of queue full
        }
        if (i % 3 == 0)
        {
            while (mEndpoint2->RecvPDU(out)) {
                ASSERT_EQ(*pdu, out);
            }
        }
    }

    teardownDefaultFDPair();
}

INSTANTIATE_TEST_CASE_P(PDUPayloads,
                        PDUPeerEndpointFDUnitTest,
                        ::testing::Values(1, 16384, 114688));


//TODO: this would be a useful tests
// TEST_F(PDUPeerSetImplOnBoxTest, ExpandsAndCopiesRingBufferWhenFull)
// {
//     mMonitor.reset(new EPollMonitor);
//     mPDUQueue1.reset(new PDUQueue);
//     mPDUQueue2.reset(new PDUQueue);

//     mMonitor->Start();

//     int fds[2];
//     socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
//     mFD1 = fds[0];
//     mFD2 = fds[1];

//     mEndpoint1.reset(
//         new PDUPeerEndpointFD(mPDUQueue1,
//                               mMonitor,
//                               TODO: change queue sizes
//                               ));
//     mEndpoint2.reset(
//         new PDUPeerEndpointFD(mPDUQueue2,
//                               mMonitor,
//                               TODO: change queue sizes
//                               ));

//     mEndpoint1->SetEventCallback(
//         boost::bind(
//             &PDUPeerEndpointFDUnitTest::EventCallback, this, _1));

//     mEndpoint2->SetEventCallback(
//         boost::bind(
//             &PDUPeerEndpointFDUnitTest::EventCallback, this, _1));

//     mEndpoint1->SetFD(mFD1);
//     mEndpoint2->SetFD(mFD2);

//     mEndpoint1->Start();
//     mEndpoint2->Start();

//     //TODO: test goes here
//     mPDUQueue1->EnqueuePDU(pdu);
//     while (!mEndpoint2->RecvPDU(out));
//     ASSERT_EQ(*pdu, out);
//
//teardownDefaultFDPair()
// }

TEST_F(PDUPeerEndpointFDUnitTest, CanSendAndVerifyAnythingMathingPDUSpec)
{
    FTRACE;

    setupDefaultFDPair();

    Forte::PDUPtr pdu = makeRandomPDU();
    Forte::PDU out;

    //TODO: onbox test for (int i=0; i<65536; ++i)
    for (int i=0; i<256; ++i)
    {
        mPDUQueue1->EnqueuePDU(pdu);
        ASSERT_NO_THROW(while (!mEndpoint2->RecvPDU(out)));
        ASSERT_EQ(*pdu, out);
    }

    teardownDefaultFDPair();
}
