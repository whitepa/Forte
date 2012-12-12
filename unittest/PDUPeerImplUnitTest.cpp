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
using ::testing::Throw;

LogManager logManager;

struct PDU_Test
{
    int a;
    int b;
    int c;
    char d[128];
} __attribute__((__packed__));

void makeTestPDU(Forte::PDU &pdu, size_t &len)
{
    PDU_Test *testPDU = reinterpret_cast<PDU_Test*>(pdu.payload);
    pdu.opcode = 1;
    pdu.payloadSize = sizeof(PDU_Test);
    testPDU->a = 1;
    testPDU->b = 2;
    testPDU->c = 3;
    snprintf(testPDU->d, sizeof(testPDU->d), "111.222.333.444");
    len = sizeof(Forte::PDU) - Forte::PDU::PDU_MAX_PAYLOAD;
    len += pdu.payloadSize;
}
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

TEST_F(PDUPeerImplUnitTest, CanConstructWithMockEndpoint)
{
    FTRACE;
    DispatcherPtr dispatcher(new GMockDispatcher());
    PDUPeerEndpointPtr endpoint(new GMockPDUPeerEndpoint());
    PDUPeerImpl peer(0, dispatcher, endpoint);
}

TEST_F(PDUPeerImplUnitTest, CanEnqueuPDUsAysnc)
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

TEST_F(PDUPeerImplUnitTest, SendNextPDUSendsFrontOfPDUList)
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

    EXPECT_CALL(*mockDispatcher, Enqueue(_)).Times(2);

    const long sendTimeout(1);

    PDUPeerImplPtr peer(new PDUPeerImpl(0, dispatcher, endpoint, sendTimeout));
    peer->SetEventCallback(
        boost::bind(&PDUPeerImplUnitTest::EventCallback, this, _1));

    peer->EnqueuePDU(pdu);

    // TODO: may want to verify this is the right pdu
    EXPECT_CALL(*mockEndpoint, SendPDU(_))
        .WillRepeatedly(Throw(EPDUPeerEndpoint()));

    // this will be called by a worker thread
    peer->SendNextPDU();
    //Timeout is 1 second
    EXPECT_FALSE(mPDUSendErrorCallbackCalled);

    sleep(1);
    peer->SendNextPDU();
    EXPECT_TRUE(mPDUSendErrorCallbackCalled);
}
