#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"
#include "GMockDispatcher.h"

#include "PDUPeerNetworkConnectorEndpoint.h"

using namespace std;
using namespace boost;
using namespace Forte;

using ::testing::_;

LogManager logManager;

class PDUPeerNetworkConnectorEndpointUnitTest : public ::testing::Test
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
        mOwnerID = 123456789;
        mDispatcher.reset(new GMockDispatcher());
    }

    void TearDown() {
    }

    uint64_t mOwnerID;
    GMockDispatcherPtr mDispatcher;
};

TEST_F(PDUPeerNetworkConnectorEndpointUnitTest, ConstructDelete)
{
    FTRACE;
    SocketAddress mListeningSocketAddress = make_pair("127.0.0.1", 12001);
    PDUPeerNetworkConnectorEndpoint theClass(mOwnerID,
                                             mDispatcher,
                                             mListeningSocketAddress);
}

TEST_F(PDUPeerNetworkConnectorEndpointUnitTest,
       CheckConnectionsWillThrowAndReenqueueEventIfItCannotConnect)
{
    FTRACE;
    SocketAddress mListeningSocketAddress = make_pair("127.0.0.1", 12001);
    PDUPeerNetworkConnectorEndpointPtr theClass(
        new PDUPeerNetworkConnectorEndpoint(mOwnerID,
                                            mDispatcher,
                                            mListeningSocketAddress));

    EXPECT_CALL(*mDispatcher, Enqueue(_));
    theClass->CheckConnections();
}

TEST_F(PDUPeerNetworkConnectorEndpointUnitTest,
       SendPDUEnqueuesCheckConnectionEventAndSetFDToInvalidOnFail)
{
    FTRACE;
    SocketAddress mListeningSocketAddress = make_pair("127.0.0.1", 12001);
    PDUPeerNetworkConnectorEndpointPtr theClass(
        new PDUPeerNetworkConnectorEndpoint(mOwnerID,
                                            mDispatcher,
                                            mListeningSocketAddress));
    PDU pdu;
    EXPECT_CALL(*mDispatcher, Enqueue(_));

    EXPECT_THROW(theClass->SendPDU(pdu), ECouldNotConnect);

    EXPECT_EQ(-1, theClass->GetFD());
}

