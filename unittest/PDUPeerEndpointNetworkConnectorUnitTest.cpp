#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"
#include "GMockDispatcher.h"

#include "PDUPeerEndpointNetworkConnector.h"

using namespace std;
using namespace boost;
using namespace Forte;

using ::testing::_;

LogManager logManager;

class PDUPeerEndpointNetworkConnectorUnitTest : public ::testing::Test
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
    }

    void TearDown() {
    }

    uint64_t mOwnerID;
};

TEST_F(PDUPeerEndpointNetworkConnectorUnitTest, ConstructDelete)
{
    FTRACE;
    boost::shared_ptr<PDUQueue> pduQueue(new PDUQueue);
    boost::shared_ptr<EPollMonitor> monitor(new EPollMonitor);

    SocketAddress mListeningSocketAddress = make_pair("127.0.0.1", 12888);
    PDUPeerEndpointNetworkConnector theClass(
        pduQueue,
        mOwnerID,
        mListeningSocketAddress,
        monitor);

    EXPECT_EQ(-1, theClass.GetFD());
}
