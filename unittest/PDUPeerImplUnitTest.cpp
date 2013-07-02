#include <sys/epoll.h>

#include "gtest/gtest.h"

#include "FTrace.h"
#include "LogManager.h"

#include "PDUPeerEndpointFactory.h"
#include "PDUPeerInProcessEndpoint.h"
#include "PDUPeerImpl.h"

#include "GMockPDUPeerEndpoint.h"

using namespace std;
using namespace boost;
using namespace Forte;

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Throw;
using ::testing::Return;

LogManager logManager;

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


class PDUPeerImplUnitTest : public ::testing::Test
{
public:
    PDUPeerImplUnitTest()
        : mEventReceivedCondition(mEventMutex)
        {
        }

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
        mReceiveEventCount = 0;
        mSendErrorEventCount = 0;
        mConnectedEventCount = 0;
        mDisconnectedEventCount = 0;
    }

    void TearDown() {
        if (mConnectedEventCount != mDisconnectedEventCount)
        {
            hlog(HLOG_INFO, "connect event count != disconnected event count");
        }
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


    boost::shared_ptr<PDU> makeTestPDU() {
        PDUPtr pdu;

        PDUHeader header;
        header.version = Forte::PDU::PDU_VERSION;
        header.opcode = 1;
        header.payloadSize = sizeof(TestPayload);

        boost::shared_array<char> optionalDataPayload(new char[1234]);
        memset(optionalDataPayload.get(), 'l', 1234);
        boost::shared_ptr<PDUOptionalData> o(
            new PDUOptionalData(1234, 0, optionalDataPayload.get()));

        TestPayload payload;
        payload.a = 32947;
        payload.b = 53947;
        payload.c = 3;
        snprintf(payload.d, sizeof(payload.d), "111.222.333.444");

        pdu.reset(new PDU(header.opcode, sizeof(payload), &payload));
        pdu->SetOptionalData(o);

        return pdu;
    }

    Forte::Mutex mEventMutex;
    Forte::ThreadCondition mEventReceivedCondition;
    int mReceiveEventCount;
    int mSendErrorEventCount;
    int mConnectedEventCount;
    int mDisconnectedEventCount;
};

TEST_F(PDUPeerImplUnitTest, CanConstructWithMockEndpoint)
{
    FTRACE;
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());
    boost::shared_ptr<PDUQueue> pduQueue(new PDUQueue);

    PDUPeerImpl peer(0, endpoint, pduQueue);
}

TEST_F(PDUPeerImplUnitTest, EnqueuePDU_ProxiesTo_PDUQueue)
{
    FTRACE;
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());
    boost::shared_ptr<PDUQueue> pduQueue(new PDUQueue);

    PDUPeerImplPtr peer(new PDUPeerImpl(0, endpoint, pduQueue));

    PDUPtr pdu = makeTestPDU();
    peer->EnqueuePDU(pdu);
    ASSERT_EQ(1, peer->GetQueueSize());
    PDUPtr pduQueued;
    pduQueue->GetNextPDU(pduQueued);
    ASSERT_EQ(pdu, pduQueued);
}

TEST_F(PDUPeerImplUnitTest, IsConnected_ProxiesTo_Endpoint)
{
    FTRACE;

    GMockPDUPeerEndpointPtr mockEndpoint(new GMockPDUPeerEndpoint());
    PDUPeerEndpointPtr endpoint = mockEndpoint;

    boost::shared_ptr<PDUQueue> pduQueue(new PDUQueue);

    PDUPeerImplPtr peer(new PDUPeerImpl(0, endpoint, pduQueue));

    PDUPtr pdu = makeTestPDU();
    peer->EnqueuePDU(pdu);
    ASSERT_EQ(1, peer->GetQueueSize());

    EXPECT_CALL(*mockEndpoint, IsConnected())
        .WillRepeatedly(Return(true));

    EXPECT_TRUE(peer->IsConnected());
}

/*
TEST_F(PDUPeerImplUnitTest, SendPDUCallsErrorCallbackOnFailure)
{
    FTRACE;
    GMockPDUPeerEndpointPtr mockEndpoint(new GMockPDUPeerEndpoint());
    PDUPeerEndpointPtr endpoint = mockEndpoint;

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));

    const long sendTimeout(0);

    PDUPeerImplPtr peer(new PDUPeerImpl(0, endpoint, sendTimeout));

    peer->EnqueuePDU(pdu);

    // TODO: may want to verify this is the right pdu
    EXPECT_CALL(*mockEndpoint, SendPDU(_))
        .WillRepeatedly(Throw(EPDUPeerEndpoint()));

    EXPECT_CALL(*mockEndpoint, IsConnected())
        .WillRepeatedly(Return(true));

    peer->SetEventCallback(
        boost::bind(&PDUPeerImplUnitTest::EventCallback, this, _1));
    peer->Start();
    {
        AutoUnlockMutex lock(mEventMutex);
        while (mEventCount == 0)
        {
            mEventCalledCondition.TimedWait(2);
        }
    }
    peer->Shutdown();

    EXPECT_TRUE(mPDUSendErrorCallbackCalled);
}

TEST_F(PDUPeerImplUnitTest, CallsErrorCallbackWhenEndpointIsNotConnected)
{
    FTRACE;
    GMockPDUPeerEndpointPtr mockEndpoint(new GMockPDUPeerEndpoint());
    PDUPeerEndpointPtr endpoint = mockEndpoint;

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));

    const long sendTimeout(0);

    PDUPeerImplPtr peer(new PDUPeerImpl(0, endpoint, sendTimeout));

    peer->EnqueuePDU(pdu);

    // TODO: may want to verify this is the right pdu
    EXPECT_CALL(*mockEndpoint, SendPDU(_))
        .WillRepeatedly(Throw(EPDUPeerEndpoint()));

    EXPECT_CALL(*mockEndpoint, IsConnected())
        .WillRepeatedly(Return(false));

    peer->SetEventCallback(
        boost::bind(&PDUPeerImplUnitTest::EventCallback, this, _1));
    peer->Start();
    {
        AutoUnlockMutex lock(mEventMutex);
        while (mEventCount == 0)
        {
            mEventCalledCondition.TimedWait(2);
        }
    }
    peer->Shutdown();

    EXPECT_TRUE(mPDUSendErrorCallbackCalled);
}

TEST_F(PDUPeerImplUnitTest, QueueOverflowCallback)
{
    FTRACE;
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));
    PDUPeerImplPtr peer(new PDUPeerImpl(0, endpoint, 1, 2,
                                        PDU_PEER_QUEUE_CALLBACK));

    peer->SetEventCallback(
        boost::bind(&PDUPeerImplUnitTest::EventCallback, this, _1));

    int count = 0;
    while (count++ < 3)
    {
        EXPECT_FALSE(mPDUSendErrorCallbackCalled);
        peer->EnqueuePDU(pdu);
    }
    EXPECT_TRUE(mPDUSendErrorCallbackCalled);
}

TEST_F(PDUPeerImplUnitTest, QueueOverflowBlock)
{
    FTRACE;
    GMockPDUPeerEndpointPtr mockEndpoint(new GMockPDUPeerEndpoint());
    PDUPeerEndpointPtr endpoint = mockEndpoint;

    // TODO: may want to verify this is the right pdu
    EXPECT_CALL(*mockEndpoint, SendPDU(_))
        .WillRepeatedly(Return());
    EXPECT_CALL(*mockEndpoint, IsConnected())
        .WillRepeatedly(Return(true));

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));
    PDUPeerImplPtr peer(new PDUPeerImpl(0, endpoint, 1, 2,
                                        PDU_PEER_QUEUE_BLOCK));

    peer->SetEventCallback(
        boost::bind(&PDUPeerImplUnitTest::EventCallback, this, _1));
    peer->Start();

    int count = 0;
    while (count++ < 6)
    {
        hlog(HLOG_DEBUG2, "Block queueing: %d", count);
        peer->EnqueuePDU(pdu);
        hlog(HLOG_DEBUG2, "Block queued: %d", count);
    }

    peer->Shutdown();

    EXPECT_FALSE(mPDUSendErrorCallbackCalled);
}

TEST_F(PDUPeerImplUnitTest, QueueOverflowThrow)
{
    FTRACE;
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));
    PDUPeerImplPtr peer(new PDUPeerImpl(0, endpoint, 1, 2,
                                        PDU_PEER_QUEUE_THROW));
    int count = 0;
    while (count++ < 2)
        peer->EnqueuePDU(pdu);
    ASSERT_THROW(peer->EnqueuePDU(pdu), EPDUPeerQueueFull);
}
*/
