// #SCQAD TESTTAG: forte, forte.pdupeer
// #SCQAD TEST: ONBOX: PDUPeerSetImplOnBoxTest

#include <sys/epoll.h>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "FTrace.h"
#include "LogManager.h"

#include "PDUPeerTypes.h"
#include "PDUPeerSet.h"
#include "PDUPeer.h"
#include "PDUPeerImpl.h"
#include "PDUPeerSetImpl.h"
#include "PDUPeerSetConnectionHandler.h"
#include "PDUPeerNetworkConnectorEndpoint.h"

#include "ReceiverThread.h"
#include "OnDemandDispatcher.h"
#include "ThreadPoolDispatcher.h"
#include "AutoMutex.h"

using namespace Forte;

using ::testing::_;

Forte::LogManager logManager;

class PDUPeerSetImplOnBoxTest : public ::testing::Test
{
public:
    PDUPeerSetImplOnBoxTest()
        : mConnectCondition(mConnectMutex),
          mReceivedCondition(mReceivedMutex)
        {}

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
        mConnectCount = 0;
        mReceivedPDUCount = 0;
    }

    void TearDown() {
    }

    void setupPDUPeerSetPairAndPeers() {
        std::vector<boost::shared_ptr<PDUPeer> > emptyPeerVector;

        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

        mEPollMonitor.reset(new EPollMonitor);
        mEPollMonitor->Start();

        mPeerSet1.reset(new PDUPeerSetImpl(emptyPeerVector, mEPollMonitor));
        mPeerSet2.reset(new PDUPeerSetImpl(emptyPeerVector, mEPollMonitor));

        mPeerSet1->Start();
        mPeerSet2->Start();

        mPeer1 = mPeerSet1->PeerCreate(fds[0]);
        mPeer2 = mPeerSet2->PeerCreate(fds[1]);

        ASSERT_EQ(1, mPeerSet1->GetConnectedCount());
        ASSERT_EQ(1, mPeerSet2->GetConnectedCount());
    }

    void teardownPDUPeerSetPair() {
        //release pointers to peers if needed. will be nice if
        //consumers to not need to worry about this
        //mPeer1.reset();
        //mPeer2.reset();

        mPeerSet1->Shutdown();
        mPeerSet2->Shutdown();
        mEPollMonitor->Shutdown();
    }


    void EventCallback(PDUPeerEventPtr event) {
        switch (event->mEventType)
        {
        case PDUPeerReceivedPDUEvent:
        {
            hlog(HLOG_INFO, "PDUPeerReceivedPDUEvent");
            PDU pdu;
            while (event->mPeer->RecvPDU(pdu))
            {
            }


            AutoUnlockMutex lock(mReceivedMutex);
            mReceivedPDUCount++;
            mReceivedCondition.Signal();
        }
        break;

        case PDUPeerSendErrorEvent:
            break;

        case PDUPeerConnectedEvent:
        {
            AutoUnlockMutex lock(mConnectMutex);
            mConnectCount++;
            mConnectCondition.Signal();
        }
        break;

        case PDUPeerDisconnectedEvent:
        default:
            break;
        }
    }

    Forte::Mutex mConnectMutex;
    Forte::ThreadCondition mConnectCondition;
    int mConnectCount;

    Forte::Mutex mReceivedMutex;
    Forte::ThreadCondition mReceivedCondition;
    int mReceivedPDUCount;

    boost::shared_ptr<EPollMonitor> mEPollMonitor;

    boost::shared_ptr<Forte::PDUPeer> mPeer1;
    boost::shared_ptr<Forte::PDUPeer> mPeer2;

    boost::shared_ptr<Forte::PDUPeerSetImpl> mPeerSet1;
    boost::shared_ptr<Forte::PDUPeerSetImpl> mPeerSet2;
};

TEST_F(PDUPeerSetImplOnBoxTest, ConstructDestruct)
{
    Forte::ThreadPoolDispatcherPtr workDispatcher;
    std::vector<boost::shared_ptr<PDUPeer> > peers;
    boost::shared_ptr<EPollMonitor> epollMonitor(new EPollMonitor);
    PDUPeerSetImpl peerSet(peers, epollMonitor);
}

TEST_F(PDUPeerSetImplOnBoxTest, StartShutdown)
{
    Forte::ThreadPoolDispatcherPtr workDispatcher;
    std::vector<boost::shared_ptr<PDUPeer> > peers;
    boost::shared_ptr<EPollMonitor> epollMonitor(new EPollMonitor);
    epollMonitor->Start();
    PDUPeerSetImpl peerSet(peers, epollMonitor);
    peerSet.Start();
    peerSet.Shutdown();
    epollMonitor->Shutdown();
}

TEST_F(PDUPeerSetImplOnBoxTest, MultiStartShutdown)
{
    Forte::ThreadPoolDispatcherPtr workDispatcher;
    std::vector<boost::shared_ptr<PDUPeer> > peers;
    boost::shared_ptr<EPollMonitor> epollMonitor(new EPollMonitor);
    PDUPeerSetImpl peerSet(peers, epollMonitor);
    peerSet.Start();
    peerSet.Shutdown();

    peerSet.Start();
    peerSet.Shutdown();
}

TEST_F(PDUPeerSetImplOnBoxTest, PeerSetMustBeRunningToCreatePeers)
{
    std::vector<boost::shared_ptr<PDUPeer> > emptyPeerVector;

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    boost::shared_ptr<EPollMonitor> epollMonitor(new EPollMonitor);

    Forte::PDUPeerSetImpl peerSet1(emptyPeerVector, epollMonitor);
    Forte::PDUPeerSetImpl peerSet2(emptyPeerVector, epollMonitor);

    ASSERT_THROW(peerSet1.PeerCreate(fds[0]), EObjectNotRunning);
    ASSERT_THROW(peerSet2.PeerCreate(fds[1]), EObjectNotRunning);
}

TEST_F(PDUPeerSetImplOnBoxTest, CanCreatePeersFromFDAfterStart)
{
    std::vector<boost::shared_ptr<PDUPeer> > emptyPeerVector;

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    boost::shared_ptr<EPollMonitor> epollMonitor(new EPollMonitor);
    epollMonitor->Start();
    Forte::PDUPeerSetImpl peerSet1(emptyPeerVector, epollMonitor);
    Forte::PDUPeerSetImpl peerSet2(emptyPeerVector, epollMonitor);

    peerSet1.Start();
    peerSet2.Start();

    boost::shared_ptr<Forte::PDUPeer> peer1 = peerSet1.PeerCreate(fds[0]);
    boost::shared_ptr<Forte::PDUPeer> peer2 = peerSet2.PeerCreate(fds[1]);

    ASSERT_EQ(1, peerSet1.GetConnectedCount());
    ASSERT_EQ(1, peerSet2.GetConnectedCount());

    peerSet1.Shutdown();
    peerSet2.Shutdown();
    epollMonitor->Shutdown();
}

TEST_F(PDUPeerSetImplOnBoxTest, DefaultSetupWorks)
{
    setupPDUPeerSetPairAndPeers();
    teardownPDUPeerSetPair();
}

TEST_F(PDUPeerSetImplOnBoxTest, CanSendAndRecvPDUs)
{
    setupPDUPeerSetPairAndPeers();

    PDUPtr pdu(new PDU(1));
    PDU pdu2;
    mPeer1->EnqueuePDU(pdu);

    while (!mPeer2->RecvPDU(pdu2)) {}
    EXPECT_EQ(*pdu, pdu2);

    teardownPDUPeerSetPair();
}

