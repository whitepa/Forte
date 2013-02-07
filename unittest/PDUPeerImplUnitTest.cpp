#include "gtest/gtest.h"

#include "FTrace.h"
#include "LogManager.h"

#include "PDUPeerEndpointFactory.h"
#include "PDUPeerInProcessEndpoint.h"
#include "PDUPeerImpl.h"

#include "GMockPDUPeerEndpoint.h"
#include "GMockDispatcher.h"

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
    }
    bool mPDUSendErrorCallbackCalled;
};

void* AsyncSendNextPDU(void *arg) {
    PDUPeerImplPtr *peer = (PDUPeerImplPtr*)(arg);
    int count = 0;
    sleep(3);
    while (count++ < 4)
    {
        sleep(2);
        hlog(HLOG_DEBUG2, "Calling SendNextPDU async: %d", count);
        (*peer)->SendNextPDU();
        hlog(HLOG_DEBUG2, "Sent SendNextPDU async: %d", count);
    }
    return peer;
}

TEST_F(PDUPeerImplUnitTest, CanConstructWithMockEndpoint)
{
    FTRACE;
    DispatcherPtr dispatcher(new GMockDispatcher());
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());
    PDUPeerImpl peer(0, dispatcher, endpoint);
}

TEST_F(PDUPeerImplUnitTest, CanEnqueuePDUsAysnc)
{
    FTRACE;
    GMockDispatcherPtr mockDispatcher(new GMockDispatcher());
    DispatcherPtr dispatcher = mockDispatcher;
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));

    // TODO: may want to verify this is the right event
    EXPECT_CALL(*mockDispatcher, Enqueue(_));

    PDUPeerImplPtr peer(new PDUPeerImpl(0, dispatcher, endpoint));
    peer->EnqueuePDU(pdu);
}

TEST_F(PDUPeerImplUnitTest, SendNextPDUSendsAll)
{
    FTRACE;
    GMockDispatcherPtr mockDispatcher(new GMockDispatcher());
    DispatcherPtr dispatcher = mockDispatcher;
    GMockPDUPeerEndpointPtr mockEndpoint(new GMockPDUPeerEndpoint());
    PDUPeerEndpointPtr endpoint = mockEndpoint;

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));

    // TODO: may want to verify this is the right event
    EXPECT_CALL(*mockDispatcher, Enqueue(_));

    PDUPeerImplPtr peer(new PDUPeerImpl(0, dispatcher, endpoint));
    peer->EnqueuePDU(pdu);

    // TODO: may want to verify this is the right pdu
    EXPECT_CALL(*mockEndpoint, SendPDU(_));
    EXPECT_CALL(*mockEndpoint, IsConnected())
        .WillRepeatedly(Return(true));
    // this will be called by a worker thread
    peer->SendNextPDU();
}

TEST_F(PDUPeerImplUnitTest, SendPDUCallsErrorCallbackAfterTimeout)
{
    FTRACE;
    GMockDispatcherPtr mockDispatcher(new GMockDispatcher());
    DispatcherPtr dispatcher = mockDispatcher;
    GMockPDUPeerEndpointPtr mockEndpoint(new GMockPDUPeerEndpoint());
    PDUPeerEndpointPtr endpoint = mockEndpoint;

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));

    const long sendTimeout(1);

    PDUPeerImplPtr peer(new PDUPeerImpl(0, dispatcher, endpoint, sendTimeout));
    peer->SetEventCallback(
        boost::bind(&PDUPeerImplUnitTest::EventCallback, this, _1));

    // PDUPeer will enqueue an event if there are still things to send
    // in the event queue after it returns, also once up front
    EXPECT_CALL(*mockDispatcher, Enqueue(_)).Times(2);

    peer->EnqueuePDU(pdu);

    // TODO: may want to verify this is the right pdu
    EXPECT_CALL(*mockEndpoint, SendPDU(_))
        .WillRepeatedly(Throw(EPDUPeerEndpoint()));

    EXPECT_CALL(*mockEndpoint, IsConnected())
        .WillRepeatedly(Return(true));

    // this will be called by a worker thread
    peer->SendNextPDU();
    //Timeout is 1 second
    EXPECT_FALSE(mPDUSendErrorCallbackCalled);

    sleep(1);
    peer->SendNextPDU();
    EXPECT_TRUE(mPDUSendErrorCallbackCalled);
}

TEST_F(PDUPeerImplUnitTest, TimesOutWhenNodeIsNotConnected)
{
    FTRACE;
    GMockDispatcherPtr mockDispatcher(new GMockDispatcher());
    DispatcherPtr dispatcher = mockDispatcher;
    GMockPDUPeerEndpointPtr mockEndpoint(new GMockPDUPeerEndpoint());
    PDUPeerEndpointPtr endpoint = mockEndpoint;

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));

    const long sendTimeout(1);

    PDUPeerImplPtr peer(new PDUPeerImpl(0, dispatcher, endpoint, sendTimeout));
    peer->SetEventCallback(
        boost::bind(&PDUPeerImplUnitTest::EventCallback, this, _1));

    // PDUPeer will enqueue an event if there are still things to send
    // in the event queue after it returns, also once up front
    EXPECT_CALL(*mockDispatcher, Enqueue(_)).Times(2);

    peer->EnqueuePDU(pdu);

    // TODO: may want to verify this is the right pdu
    EXPECT_CALL(*mockEndpoint, SendPDU(_))
        .WillRepeatedly(Throw(EPDUPeerEndpoint()));

    EXPECT_CALL(*mockEndpoint, IsConnected())
        .WillRepeatedly(Return(false));

    // this will be called by a worker thread
    peer->SendNextPDU();
    //Timeout is 1 second
    EXPECT_FALSE(mPDUSendErrorCallbackCalled);

    sleep(1);
    peer->SendNextPDU();
    EXPECT_TRUE(mPDUSendErrorCallbackCalled);
}

TEST_F(PDUPeerImplUnitTest, QueueOverflowBlock)
{
    FTRACE;
    GMockDispatcherPtr mockDispatcher(new GMockDispatcher());
    DispatcherPtr dispatcher = mockDispatcher;
    GMockPDUPeerEndpointPtr mockEndpoint(new GMockPDUPeerEndpoint());
    PDUPeerEndpointPtr endpoint = mockEndpoint;

    // TODO: may want to verify this is the right event
    EXPECT_CALL(*mockDispatcher, Enqueue(_)).Times(AtLeast(1));

    // TODO: may want to verify this is the right pdu
    EXPECT_CALL(*mockEndpoint, SendPDU(_))
        .WillRepeatedly(Return());
    EXPECT_CALL(*mockEndpoint, IsConnected())
        .WillRepeatedly(Return(true));

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));
    PDUPeerImplPtr peer(new PDUPeerImpl(0, dispatcher, endpoint, 1, 2,
                                        PDU_PEER_QUEUE_BLOCK));

    // Create thread to async send pdus and free up queue space
    // This test will block indefinitely if it fails.
    pthread_t sendThread;
    int res = pthread_create(&sendThread, NULL, &AsyncSendNextPDU, &peer);
    EXPECT_EQ(0, res);

    int count = 0;
    while (count++ < 6)
    {
        hlog(HLOG_DEBUG2, "Block queueing: %d", count);
        peer->EnqueuePDU(pdu);
        hlog(HLOG_DEBUG2, "Block queued: %d", count);
    }
    EXPECT_EQ(0, pthread_join(sendThread, NULL));
}

TEST_F(PDUPeerImplUnitTest, QueueOverflowCallback)
{
    FTRACE;
    GMockDispatcherPtr mockDispatcher(new GMockDispatcher());
    DispatcherPtr dispatcher = mockDispatcher;
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());

    // TODO: may want to verify this is the right event
    EXPECT_CALL(*mockDispatcher, Enqueue(_)).Times(AtLeast(1));

    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));
    PDUPeerImplPtr peer(new PDUPeerImpl(0, dispatcher, endpoint, 1, 2,
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

TEST_F(PDUPeerImplUnitTest, QueueOverflowThrow)
{
    FTRACE;
    GMockDispatcherPtr mockDispatcher(new GMockDispatcher());
    DispatcherPtr dispatcher = mockDispatcher;
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());

    // TODO: may want to verify this is the right event
    EXPECT_CALL(*mockDispatcher, Enqueue(_)).Times(AtLeast(1));


    int op = 1;
    char buf[] = "the data";
    PDUPtr pdu(new PDU(op, sizeof(buf), buf));
    PDUPeerImplPtr peer(new PDUPeerImpl(0, dispatcher, endpoint, 1, 2,
                                        PDU_PEER_QUEUE_THROW));
    int count = 0;
    while (count++ < 2)
        peer->EnqueuePDU(pdu);
    ASSERT_THROW(peer->EnqueuePDU(pdu), EPDUPeerQueueFull);
}
