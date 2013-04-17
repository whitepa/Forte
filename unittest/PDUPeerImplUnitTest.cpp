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

struct PDU_Test
{
    int a;
    int b;
    int c;
    char d[128];
} __attribute__((__packed__));

// -----------------------------------------------------------------------------

class PDUPeerImplUnitTest : public ::testing::Test
{
public:
    PDUPeerImplUnitTest()
        : mEventMutex(),
          mEventCalledCondition(mEventMutex),
          mEventCount(0)
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
        mPDUSendErrorCallbackCalled = false;
    }

    void TearDown() {
    }

    void EventCallback(PDUPeerEventPtr event) {
        switch (event->mEventType)
        {
        case PDUPeerReceivedPDUEvent:
            break;

        case PDUPeerSendErrorEvent:
            mPDUSendErrorCallbackCalled = true;
            break;

        case PDUPeerConnectedEvent:
        case PDUPeerDisconnectedEvent:
        default:
            break;
        }

        AutoUnlockMutex lock(mEventMutex);
        mEventCount++;
        mEventCalledCondition.Signal();
    }

    bool mPDUSendErrorCallbackCalled;

    mutable Forte::Mutex mEventMutex;
    mutable Forte::ThreadCondition mEventCalledCondition;
    int mEventCount;
};

TEST_F(PDUPeerImplUnitTest, CanConstructWithMockEndpoint)
{
    FTRACE;
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());
    PDUPeerImpl peer(0, endpoint);
}

TEST_F(PDUPeerImplUnitTest, CanEnqueuePDUsAysnc)
{
    FTRACE;
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));

    PDUPeerImplPtr peer(new PDUPeerImpl(0, endpoint));
    peer->EnqueuePDU(pdu);
    ASSERT_EQ(1, peer->GetQueueSize());
}

TEST_F(PDUPeerImplUnitTest, InternalThreadSendsAfterBeginCall)
{
    FTRACE;
    GMockPDUPeerEndpointPtr mockEndpoint(new GMockPDUPeerEndpoint());
    PDUPeerEndpointPtr endpoint = mockEndpoint;

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));

    PDUPeerImplPtr peer(new PDUPeerImpl(0, endpoint));
    peer->EnqueuePDU(pdu);
    ASSERT_EQ(1, peer->GetQueueSize());

    EXPECT_CALL(*mockEndpoint, SendPDU(_));
    EXPECT_CALL(*mockEndpoint, IsConnected())
        .WillRepeatedly(Return(true));

    peer->SetEventCallback(
        boost::bind(&PDUPeerImplUnitTest::EventCallback, this, _1));
    peer->Begin();
    while (peer->GetQueueSize() > 0)
    {
        usleep(10000);
    }
    peer->Shutdown();
    ASSERT_EQ(0, peer->GetQueueSize());
}

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
    peer->Begin();
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
    peer->Begin();
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
    peer->Begin();

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
