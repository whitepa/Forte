// #SCQAD TESTTAG: forte, forte.pdupeer
// #SCQAD TEST: ONBOX: PDUPeerSetBuilderImplOnBoxTest

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "AutoFD.h"
#include "FTrace.h"
#include "LogManager.h"
#include "Types.h"
#include "PDU.h"
#include "PDUPeerSetBuilderImpl.h"
#include "PDUPeerEndpointFactoryImpl.h"

#include <boost/bind.hpp>

using namespace std;
using namespace boost;
using namespace Forte;

LogManager logManager;

struct Payload
{
    int a;
    int b;
    int c;
    char d[128];
} __attribute__((__packed__));


class TestPeer
{
public:
    TestPeer(const SocketAddress& myListenAddress,
             const SocketAddressVector& peerAddresses)
        : mCallbackCount(0),
          mConnectEventCount(0),
          mPeerAddresses(peerAddresses),
          mListenAddress(myListenAddress)
        {
            mPeerID = PDUPeerSetBuilderImpl::SocketAddressToID(myListenAddress);
            InstantiatePeerSet();
        }

    void InstantiatePeerSet() {
        const bool createInProcessPDUPeer(true);
        mPeerSet.reset(
            new PDUPeerSetBuilderImpl(
                mListenAddress,
                mPeerAddresses,
                boost::bind(&TestPeer::EventCallback, this, _1),
                2, // min threads
                4, // max threads
                createInProcessPDUPeer
                ));
    }

    void DeletePeerSet() {
        mPeerSet.reset();
    }

    void EventCallback(PDUPeerEventPtr event) {
        AutoUnlockMutex lock(mEventCallbackMutex);

        switch (event->mEventType)
        {
        case PDUPeerReceivedPDUEvent:
        {
            //hlog(HLOG_DEBUG, "PDUPeerReceivedPDUEvent");
            PDUPtr receivedPDUPtr(new PDU);
            while (event->mPeer->RecvPDU(*receivedPDUPtr))
            {
                AutoUnlockMutex lock(mPeerDataMutex);
                mReceivedPDUList.push_back(receivedPDUPtr);
                mCallbackCount++;
            }
        }
        break;

        case PDUPeerSendErrorEvent:
            break;

        case PDUPeerConnectedEvent:
            mConnectEventCount++;
            hlogstream(HLOG_INFO, "connected event for " << mPeerID);
            break;

        case PDUPeerDisconnectedEvent:
        default:
            break;
        }
    }

    int GetConnectedCount() {
        AutoUnlockMutex lock(mPeerDataMutex);
        return mCallbackCount;
    }

    PDUPeerSetBuilderPtr mPeerSet;
    uint64_t mPeerID;
    Forte::Mutex mPeerDataMutex;
    std::list<PDUPtr> mReceivedPDUList;

    Forte::Mutex mEventCallbackMutex;
    int mCallbackCount;
    int mConnectEventCount;

    SocketAddressVector mPeerAddresses;
    SocketAddress mListenAddress;
};

typedef boost::shared_ptr<TestPeer> TestPeerPtr;

class PDUPeerSetBuilderImplOnBoxTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        //logManager.BeginLogging("//stdout", HLOG_ALL);
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
    }

    static void TearDownTestCase() {
        logManager.EndLogging();
    }

    void SetUp() {
        hlog(HLOG_INFO, "Starting test...");
    }

    void TearDown() {
        hlog(HLOG_INFO, "Ending test...");
    }

    void expectEqual(const PDUPtr& expected, const PDUPtr& received) {
        EXPECT_EQ(expected->GetOpcode(), received->GetOpcode());
        EXPECT_EQ(expected->GetPayloadSize(), received->GetPayloadSize());

        const Payload *expectedPayload = expected->GetPayload<Payload>();
        const Payload *receivedPayload = received->GetPayload<Payload>();

        EXPECT_EQ(expectedPayload->a, receivedPayload->a);
        EXPECT_EQ(expectedPayload->b, receivedPayload->b);
        EXPECT_EQ(expectedPayload->c, receivedPayload->c);
        EXPECT_STREQ(expectedPayload->d, receivedPayload->d);
    }

    PDUPtr makePDUPtr(int opcode=1) {
        PDUPtr pdu(new PDU(opcode, sizeof(Payload)));
        Payload *p = pdu->GetPayload<Payload>();

        p->a = opcode + 1;
        p->b = opcode + 2;
        p->c = opcode + 3;
        stringstream s;
        s << "opcode is " << opcode;
        strncpy(p->d, s.str().c_str(), 128);

        return pdu;
    }

    void setupTwoPeers() {
        mTestPeers.resize(2);
        setupPeer(0, 2);
        setupPeer(1, 2);
    }

    void setupThreePeers() {
        mTestPeers.resize(3);
        setupPeer(0, 3);
        setupPeer(1, 3);
        setupPeer(2, 3);
    }

    void setupPeers(int numPeers) {
        mTestPeers.resize(numPeers);
        for (int i=0; i<numPeers; i++)
        {
            setupPeer(i, numPeers);
        }
    }

    void setupPeer(int index, int total) {
        SocketAddressVector addresses;
        for (int i=0; i<total; i++)
        {
            addresses.push_back(make_pair("127.0.0.1", 13000 + i));
        }

        // caller must ensure mTestPeers is correct size
        mTestPeers[index].reset(new TestPeer(addresses[index], addresses));
    }

    bool isConnected(int peerIndex1, int peerIndex2) {
        return
            mTestPeers[peerIndex1]->mPeerSet->GetPeer(
                mTestPeers[peerIndex2]->mPeerID)->IsConnected();
    }

    std::vector<boost::shared_ptr<TestPeer> > mTestPeers;
};

TEST_F(PDUPeerSetBuilderImplOnBoxTest, IDIsIPAndPortAs64BitInt)
{
    FTRACE;
    SocketAddress listenAddress(make_pair("127.0.0.1", 13001));

    //2130706433 | 13001
    //0x7F000001 << 32 | 0x32C9
    //0x7F00000100000000 | 0x32C9
    // = 0x7F000001000032C9
    // = 9151314447111828169

    ASSERT_EQ(9151314447111828169,
              PDUPeerSetBuilderImpl::SocketAddressToID(listenAddress));
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, ConstructorSetsUpBasics)
{
    FTRACE;
    SocketAddress listenAddress(make_pair("127.0.0.1", 13001));

    SocketAddressVector addresses;
    addresses.push_back(listenAddress);
    addresses.push_back(make_pair("127.0.0.1", 13002));
    addresses.push_back(make_pair("127.0.0.1", 13003));

    PDUPeerSetBuilderImpl peerSet(listenAddress, addresses);

    //2130706433 | 13001
    //0x7F000001 << 32 | 0x32C9
    //0x7F00000100000000 | 0x32C9
    // = 0x7F000001000032C9
    // = 9151314447111828169

    ASSERT_EQ(9151314447111828169,
              PDUPeerSetBuilderImpl::SocketAddressToID(listenAddress));
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, CanDispatchWithoutThrowing)
{
    FTRACE;

    SocketAddress listenAddress(make_pair("127.0.0.1", 13001));

    SocketAddressVector addresses;
    addresses.push_back(listenAddress);
    addresses.push_back(make_pair("127.0.0.1", 13002));
    addresses.push_back(make_pair("127.0.0.1", 13003));

    PDUPeerSetBuilderImpl peerSet(listenAddress, addresses);

    PDUPtr pdu = makePDUPtr();
    peerSet.BroadcastAsync(pdu);
}


TEST_F(PDUPeerSetBuilderImplOnBoxTest, CanProvideConnectionStatusOnPeers)
{
    FTRACE;

    setupThreePeers();

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool allConnected(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            foreach(const TestPeerPtr& peer2, mTestPeers)
            {
                if (!peer->mPeerSet->GetPeer(peer2->mPeerID)->IsConnected())
                {
                    allConnected = false;
                }
            }
        }

        if (allConnected)
            break;

        usleep(10000);
    }

    EXPECT_TRUE(isConnected(0,0));
    EXPECT_TRUE(isConnected(0,1));
    EXPECT_TRUE(isConnected(0,2));

    EXPECT_TRUE(isConnected(1,0));
    EXPECT_TRUE(isConnected(1,1));
    EXPECT_TRUE(isConnected(1,2));

    EXPECT_TRUE(isConnected(2,0));
    EXPECT_TRUE(isConnected(2,1));
    EXPECT_TRUE(isConnected(2,2));

    mTestPeers[1]->mPeerSet.reset();

    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired()
           && (isConnected(0,1) || isConnected(2,1)))
    {
        usleep(10000);
    }

    EXPECT_TRUE(isConnected(0,2));
    EXPECT_TRUE(isConnected(2,0));
    EXPECT_FALSE(isConnected(0,1));
    EXPECT_FALSE(isConnected(2,1));
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, AllPeersReceiveConnectEvent)
{
    FTRACE;

    setupThreePeers();

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool allConnected(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            foreach(const TestPeerPtr& peer2, mTestPeers)
            {
                if (!peer->mPeerSet->GetPeer(peer2->mPeerID)->IsConnected())
                {
                    allConnected = false;
                }
            }
        }

        if (allConnected)
            break;

        usleep(10000);
    }

    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        EXPECT_EQ(3, peer->mConnectEventCount);
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, AllPeersReceiveABroadcastPDUFromFirst)
{
    FTRACE;
    setupThreePeers();

    PDUPtr pdu = makePDUPtr();
    mTestPeers[0]->mPeerSet->BroadcastAsync(pdu);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetConnectedCount() == 0)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    PDUPtr expected = makePDUPtr();
    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        ASSERT_EQ(1, peer->GetConnectedCount());
        ASSERT_EQ(1, peer->mReceivedPDUList.size());
        expectEqual(expected, peer->mReceivedPDUList.front());
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, AllPeersReceiveABroadcastPDUFromMiddle)
{
    FTRACE;
    setupThreePeers();

    PDUPtr pdu = makePDUPtr();
    mTestPeers[1]->mPeerSet->BroadcastAsync(pdu);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetConnectedCount() == 0)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    PDUPtr expected = makePDUPtr();
    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        ASSERT_EQ(1, peer->GetConnectedCount());
        ASSERT_EQ(1, peer->mReceivedPDUList.size());
        expectEqual(expected, peer->mReceivedPDUList.front());
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, AllPeersReceiveABroadcastPDUFromLast)
{
    FTRACE;
    setupThreePeers();

    PDUPtr pdu = makePDUPtr();
    mTestPeers[2]->mPeerSet->BroadcastAsync(pdu);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetConnectedCount() == 0)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    PDUPtr expected = makePDUPtr();
    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        ASSERT_EQ(1, peer->GetConnectedCount());
        ASSERT_EQ(1, peer->mReceivedPDUList.size());
        expectEqual(expected, peer->mReceivedPDUList.front());
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, StopsReceivingEventsIfCallbackIsDisabled)
{
    FTRACE;
    setupThreePeers();
    mTestPeers[1]->mPeerSet->SetEventCallback(NULL);

    PDUPtr pdu = makePDUPtr();
    mTestPeers[0]->mPeerSet->BroadcastAsync(pdu);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        if (mTestPeers[0]->GetConnectedCount() > 0
            && mTestPeers[2]->GetConnectedCount() > 0)
        {
            callbacksDone = false;
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    sleep(1);

    ASSERT_EQ(0, mTestPeers[1]->GetConnectedCount());
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, CanBroadcastAtLeast50PDUsFromEach)
{
    FTRACE;
    setupThreePeers();

    for (int i=0; i<50; i++)
    {
        PDUPtr pdu = makePDUPtr(i);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            peer->mPeerSet->BroadcastAsync(pdu);
        }
    }

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetConnectedCount() < 150)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        ASSERT_EQ(150, peer->GetConnectedCount());
        ASSERT_EQ(150, peer->mReceivedPDUList.size());

        while (!peer->mReceivedPDUList.empty())
        {
            PDUPtr expected = makePDUPtr(peer->mReceivedPDUList.front()->GetOpcode());
            expectEqual(expected, peer->mReceivedPDUList.front());
            peer->mReceivedPDUList.pop_front();
        }
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, PDUWillComeAcrossIfConnectorDiesAndRestarts)
{
    FTRACE;
    setupTwoPeers();

    hlog(HLOG_INFO, "Killing peer 1");
    mTestPeers[1]->mPeerSet.reset();

    hlog(HLOG_INFO, "Enqueuing 1 pdu on remaining peer");
    PDUPtr pdu = makePDUPtr();
    mTestPeers[0]->mPeerSet->GetPeer(mTestPeers[1]->mPeerID)->EnqueuePDU(pdu);

    // let it sit in the queue for a bit
    sleep(3);

    hlog(HLOG_INFO, "Recreating peer 1");
    setupPeer(1, 2);

    hlog(HLOG_INFO, "Waiting for successful finish");

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        if (mTestPeers[1]->GetConnectedCount() < 1)
        {
            callbacksDone = false;
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    ASSERT_EQ(1, mTestPeers[1]->GetConnectedCount());
    ASSERT_EQ(1, mTestPeers[1]->mReceivedPDUList.size());
    expectEqual(pdu, mTestPeers[1]->mReceivedPDUList.front());
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, PDUWillComeAcrossIfAcceptorDiesAndRestarts)
{
    FTRACE;
    setupTwoPeers();

    hlog(HLOG_INFO, "Killing peer 1");
    mTestPeers[0]->mPeerSet.reset();

    hlog(HLOG_INFO, "Enqueuing 1 pdu on remaining peer");
    PDUPtr pdu = makePDUPtr();
    mTestPeers[1]->mPeerSet->GetPeer(mTestPeers[0]->mPeerID)->EnqueuePDU(pdu);

    // let it sit in the queue for a bit
    sleep(3);

    hlog(HLOG_INFO, "Recreating peer 1");
    setupPeer(0, 2);

    hlog(HLOG_INFO, "Waiting for successful finish");

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        if (mTestPeers[0]->GetConnectedCount() < 1)
        {
            callbacksDone = false;
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    ASSERT_EQ(1, mTestPeers[0]->GetConnectedCount());
    ASSERT_EQ(1, mTestPeers[0]->mReceivedPDUList.size());
    expectEqual(pdu, mTestPeers[0]->mReceivedPDUList.front());
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, CanEnque100PDUsForDownPeer)
{
    FTRACE;
    setupTwoPeers();

    hlog(HLOG_INFO, "Killing peer 1");
    mTestPeers[0]->DeletePeerSet();

    // without waiting for disconnect, an interesting situation can
    // arise where the PDU is fully delivered to the other side but
    // the other side dies before getting any callbacks. Handling this
    // situation is outside the scope of PDUPeer
    while (isConnected(1,0))
    {
        usleep(10000);
    }

    hlog(HLOG_INFO, "Enqueuing 100 PDUs on remaining peer");

    for (int i=0; i<100; i++)
    {
        PDUPtr pdu = makePDUPtr(i);
        mTestPeers[1]->mPeerSet->GetPeer(mTestPeers[0]->mPeerID)->EnqueuePDU(pdu);
    }

    // let it sit in the queue for a bit
    sleep(3);

    hlog(HLOG_INFO, "Recreating peer 1");
    mTestPeers[0]->InstantiatePeerSet();

    hlog(HLOG_INFO, "Waiting for successful finish");

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        if (mTestPeers[0]->GetConnectedCount() < 100)
        {
            callbacksDone = false;
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    ASSERT_EQ(100, mTestPeers[0]->GetConnectedCount());
    ASSERT_EQ(100, mTestPeers[0]->mReceivedPDUList.size());
    const TestPeerPtr& peer(mTestPeers[0]);
    while (!peer->mReceivedPDUList.empty())
    {
        PDUPtr expected = makePDUPtr(peer->mReceivedPDUList.front()->GetOpcode());
        expectEqual(expected, peer->mReceivedPDUList.front());
        peer->mReceivedPDUList.pop_front();
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, PeersRecoverOnRestartsWithinTimeout)
{
    FTRACE;
    setupThreePeers();

    hlog(HLOG_INFO, "Killing peer 1");
    mTestPeers[1]->DeletePeerSet();

    while (isConnected(0,1) || isConnected(2,1))
    {
        usleep(10000);
    }

    hlog(HLOG_INFO, "Enqueuing 50 pdus on remaining peers");
    for (int i=0; i<50; i++)
    {
        PDUPtr pdu = makePDUPtr(i);
        mTestPeers[0]->mPeerSet->BroadcastAsync(pdu);
        mTestPeers[2]->mPeerSet->BroadcastAsync(pdu);
    }

    hlog(HLOG_INFO, "Waiting for PDUs to start coming in");

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            // have peer 1 come up somewhere in the middle
            if (peer->GetConnectedCount() < 3)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    hlog(HLOG_INFO, "Recreating peer 1");

    setupPeer(1, 3);

    hlog(HLOG_INFO, "Waiting for successful finish");

    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetConnectedCount() < 100)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        ASSERT_EQ(100, peer->GetConnectedCount());
        ASSERT_EQ(100, peer->mReceivedPDUList.size());

        while (!peer->mReceivedPDUList.empty())
        {
            PDUPtr expected = makePDUPtr(peer->mReceivedPDUList.front()->GetOpcode());
            expectEqual(expected, peer->mReceivedPDUList.front());
            peer->mReceivedPDUList.pop_front();
        }
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest,
       CanBroadcastAndReceive1000PDUsFromEachIn10Seconds)
{
    FTRACE;
    setupThreePeers();

    for (int i=0; i<1000; i++)
    {
        PDUPtr pdu = makePDUPtr(i);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            peer->mPeerSet->BroadcastAsync(pdu);
        }
    }

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetConnectedCount() < 3000)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        ASSERT_EQ(3000, peer->GetConnectedCount());
        ASSERT_EQ(3000, peer->mReceivedPDUList.size());

        while (!peer->mReceivedPDUList.empty())
        {
            PDUPtr expected = makePDUPtr(peer->mReceivedPDUList.front()->GetOpcode());
            expectEqual(expected, peer->mReceivedPDUList.front());
            peer->mReceivedPDUList.pop_front();
        }
    }
}

// this is more of a developer test to ensure that as long as
// something is consuming PDUs the PeerSet itself is not holding on to
// them
TEST_F(PDUPeerSetBuilderImplOnBoxTest, DISABLED_ProperlyReleasesPDUs)
{
    FTRACE;
    setupTwoPeers();

    const TestPeerPtr& broadcaster(mTestPeers[1]);
    int i = 0;
    DeadlineClock deadline;
    deadline.ExpiresInSeconds(600);
    while (!deadline.Expired())
    {
        i++;
        PDUPtr pdu = makePDUPtr(i);
        broadcaster->mPeerSet->BroadcastAsync(pdu);

        //hlog(HLOG_DEBUG, "Sent PDU");
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            while (true)
            {
                {
                    AutoUnlockMutex lock(peer->mPeerDataMutex);
                    if (peer->mReceivedPDUList.size() != 0)
                    {
                        break;
                    }
                }

                usleep(1000);
            }

            ASSERT_EQ(1, peer->mReceivedPDUList.size());
            expectEqual(pdu, peer->mReceivedPDUList.front());

            // clear what we recv'd
            peer->mReceivedPDUList.pop_front();
        }
        //hlog(HLOG_DEBUG, "All Peers received 1 matching pdu");
    }

    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        ASSERT_EQ(0, peer->mReceivedPDUList.size());
    }
    hlogstream(HLOG_INFO, "sent " << i << " pdus");
}

//This is purely to ensure PDUPeers can teardown and setup ok
//underload, no checking of PDUs sent/recv'd will be done
TEST_F(PDUPeerSetBuilderImplOnBoxTest,
       PDUPeersCanDieAndRecoverUnderLoad)
{
    FTRACE;
    setupPeers(8);

    const TestPeerPtr& broadcaster(mTestPeers[1]);
    const TestPeerPtr& flaker(mTestPeers[1]);
    int i = 0;
    DeadlineClock deadline;
    deadline.ExpiresInSeconds(600);
    while (!deadline.Expired())
    {
        i++;
        PDUPtr pdu = makePDUPtr(i);
        broadcaster->mPeerSet->BroadcastAsync(pdu);

        if (i % 100 == 0)
        {
            flaker->DeletePeerSet();
            flaker->InstantiatePeerSet();
        }

        // 1237 here just indicates i want to clear out the recv'd
        // list periodically, but i don't want it to be on the same
        // loops as as when flaker flakes
        if (i % 1237 == 0)
        {
            foreach(const TestPeerPtr& peer, mTestPeers)
            {
                peer->mReceivedPDUList.clear();
            }
        }
    }
    hlogstream(HLOG_INFO, "sent " << i << " pdus");
}
