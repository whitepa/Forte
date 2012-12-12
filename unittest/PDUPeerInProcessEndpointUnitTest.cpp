#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"
#include <boost/bind.hpp>
#include "PDUPeerInProcessEndpoint.h"

using namespace std;
using namespace boost;
using namespace Forte;

LogManager logManager;

class PDUPeerInProcessEndpointUnitTest : public ::testing::Test
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
        mCallbackReceiveCount = 0;
        mSendErrorCallbackReceiveCount = 0;
    }

    void TearDown() {
    }

    void EventCallback(PDUPeerEventPtr event) {
        switch (event->mEventType)
        {
        case PDUPeerReceivedPDUEvent:
            mCallbackReceiveCount++;
            break;

        case PDUPeerSendErrorEvent:
            mSendErrorCallbackReceiveCount++;
            break;

        case PDUPeerConnectedEvent:
        case PDUPeerDisconnectedEvent:
        default:
            break;
        }
    }

    int mCallbackReceiveCount;
    int mSendErrorCallbackReceiveCount;
};

TEST_F(PDUPeerInProcessEndpointUnitTest, ConstructDelete)
{
    FTRACE;
    PDUPeerInProcessEndpoint theClass;
}

TEST_F(PDUPeerInProcessEndpointUnitTest, ThrowsWhenPeerIsNotSetupToReceive)
{
    FTRACE;
    PDUPeerInProcessEndpoint theClass;

    PDU p;

    ASSERT_THROW(theClass.SendPDU(p), EPDUPeerEndpoint);
}

TEST_F(PDUPeerInProcessEndpointUnitTest, CanSendPDUAndGetCallback)
{
    FTRACE;
    PDUPeerInProcessEndpoint theClass;
    theClass.SetEventCallback(
        boost::bind(&PDUPeerInProcessEndpointUnitTest::EventCallback, this, _1));

    PDU p;
    theClass.SendPDU(p);
    ASSERT_EQ(1, mCallbackReceiveCount);
}

TEST_F(PDUPeerInProcessEndpointUnitTest,
       SendPDUTriggersThrowsIfCallbackIsNotSet)
{
    FTRACE;
    PDUPeerInProcessEndpoint theClass;

    PDU p;
    ASSERT_THROW(theClass.SendPDU(p), EPDUPeerEndpoint);
}
