#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"
#include <boost/bind.hpp>
#include "PDUPeerEndpointInProcess.h"

using namespace std;
using namespace boost;
using namespace Forte;

LogManager logManager;

class PDUPeerEndpointInProcessUnitTest : public ::testing::Test
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

TEST_F(PDUPeerEndpointInProcessUnitTest, ConstructDelete)
{
    FTRACE;
    boost::shared_ptr<PDUQueue> pduQueue(new PDUQueue);
    PDUPeerEndpointInProcess theClass(pduQueue);
}

TEST_F(PDUPeerEndpointInProcessUnitTest, Requires2StageConstruction)
{
    FTRACE;
    boost::shared_ptr<PDUQueue> pduQueue(new PDUQueue);
    PDUPeerEndpointInProcess theClass(pduQueue);
    theClass.SetEventCallback(
        boost::bind(&PDUPeerEndpointInProcessUnitTest::EventCallback, this, _1));

    theClass.Start();
    sleep(1);
    theClass.Shutdown();
}

TEST_F(PDUPeerEndpointInProcessUnitTest, PullsPDUsFromPDUQueueAndSendsThem)
{
    FTRACE;
    boost::shared_ptr<PDUQueue> pduQueue(new PDUQueue);
    PDUPeerEndpointInProcess theClass(pduQueue);
    theClass.SetEventCallback(
        boost::bind(&PDUPeerEndpointInProcessUnitTest::EventCallback, this, _1));
    theClass.Start();

    PDUPtr p(new PDU);
    pduQueue->EnqueuePDU(p);
    sleep(1);
    ASSERT_EQ(1, mCallbackReceiveCount);

    theClass.Shutdown();
}
