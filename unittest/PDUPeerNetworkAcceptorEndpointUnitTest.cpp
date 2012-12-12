#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"

#include "PDUPeerNetworkAcceptorEndpoint.h"

using namespace std;
using namespace boost;
using namespace Forte;

LogManager logManager;

class PDUPeerNetworkAcceptorEndpointUnitTest : public ::testing::Test
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

TEST_F(PDUPeerNetworkAcceptorEndpointUnitTest, ConstructDelete)
{
    FTRACE;
    SocketAddress s = make_pair("127.0.0.1", 11001);
    PDUPeerNetworkAcceptorEndpoint e(s);
}

TEST_F(PDUPeerNetworkAcceptorEndpointUnitTest, AcceptorThrowsIfFDNotSet)
{
    FTRACE;
    SocketAddress s = make_pair("127.0.0.1", 11001);
    PDUPeerNetworkAcceptorEndpoint e(s);

    PDU p;
    EXPECT_THROW(e.SendPDU(p), ENotConnected);
}
