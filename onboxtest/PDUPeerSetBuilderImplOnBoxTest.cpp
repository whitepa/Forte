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
        : mEventReceivedCondition(mEventMutex),
          mReceiveEventCount(0),
          mReceivePDUCount(0),
          mSendErrorEventCount(0),
          mConnectedEventCount(0),
          mDisconnectedEventCount(0),
          mPeerAddresses(peerAddresses),
          mListenAddress(myListenAddress)
        {
            mPeerID = PDUPeerSetBuilderImpl::SocketAddressToID(myListenAddress);
            InstantiatePeerSet();
        }

    ~TestPeer() {
        DeletePeerSet();
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
                createInProcessPDUPeer,
                30,
                1024,
                PDU_PEER_QUEUE_BLOCK
                ));

        mPeerSet->Start();
    }

    void DeletePeerSet() {
        if (mPeerSet)
        {
            mPeerSet->SetEventCallback(NULL);
            mPeerSet->Shutdown();
            mPeerSet.reset();
        }
    }

    void EventCallback(PDUPeerEventPtr event) {
        Forte::AutoUnlockMutex lock(mEventMutex);
        switch (event->mEventType)
        {
        case PDUPeerReceivedPDUEvent:
        {
            ++mReceiveEventCount;

            PDUPtr receivedPDUPtr(new PDU);
            while (event->mPeer->RecvPDU(*receivedPDUPtr))
            {
                mReceivedPDUList.push_back(receivedPDUPtr);
                ++mReceivePDUCount;
            }

            if (hlog_ratelimit(10))
            {
                hlogstream(HLOG_DEBUG,
                           "Recvd PDU. total so far:" << mReceivePDUCount);
            }

        }
        break;

        case PDUPeerSendErrorEvent:
            hlog(HLOG_INFO, "Recvd Send Error Event");
            ++mSendErrorEventCount;
            break;

        case PDUPeerConnectedEvent:
            hlog(HLOG_DEBUG2, "Recvd Connect Event");
            ++mConnectedEventCount;
            break;

        case PDUPeerDisconnectedEvent:
            hlog(HLOG_DEBUG2, "Recvd Disconnect Event");
            ++mDisconnectedEventCount;
            break;

        default:
            break;
        }

        mEventReceivedCondition.Signal();
    }

    int GetReceivedCount() {
        AutoUnlockMutex lock(mEventMutex);
        return mReceivePDUCount;
    }

    int GetConnectedEventCount() {
        AutoUnlockMutex lock(mEventMutex);
        return mConnectedEventCount;
    }

    int GetDisconnectedEventCount() {
        AutoUnlockMutex lock(mEventMutex);
        return mDisconnectedEventCount;
    }

    Forte::Mutex mEventMutex;
    Forte::ThreadCondition mEventReceivedCondition;
    int mReceiveEventCount;
    int mReceivePDUCount;
    int mSendErrorEventCount;
    int mConnectedEventCount;
    int mDisconnectedEventCount;
    std::list<PDUPtr> mReceivedPDUList;

    PDUPeerSetBuilderPtr mPeerSet;
    uint64_t mPeerID;

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
        EXPECT_EQ(*expected, *received);
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

    PDUPtr makePDUWithOptionDataPtr(int opcode=3,
                                    const int optionalDataSize = 100) {
        PDUPtr pdu(new PDU(opcode, sizeof(Payload)));
        Payload *p = pdu->GetPayload<Payload>();

        p->a = opcode + 1;
        p->b = opcode + 2;
        p->c = opcode + 3;
        stringstream s;
        s << "opcode is " << opcode;
        strncpy(p->d, s.str().c_str(), 128);

        boost::shared_ptr<PDUOptionalData> optionalData;
        boost::shared_array<char> data(new char[optionalDataSize]);

        for(int i = 0; i<optionalDataSize; i++)
        {
            data[i] = i % sizeof(char);
        }

        optionalData.reset(
            new PDUOptionalData(
                optionalDataSize,
                0,
                data.get()));

        pdu->SetOptionalData(optionalData);

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

    void waitForAllConnected(unsigned int timeout = 10) {
        DeadlineClock deadline;
        deadline.ExpiresInSeconds(timeout);
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

        if (deadline.Expired())
        {
            throw Exception(FStringFC(),
                            "All peers not connected in %d seconds",
                            timeout);
        }
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

TEST_F(PDUPeerSetBuilderImplOnBoxTest, IDCanBeConvertedByToIPPort)
{
    FTRACE;
    SocketAddress listenAddress(make_pair("127.0.0.1", 13001));

    ASSERT_EQ("127.0.0.1:13001",
              PDUPeerSetBuilderImpl::IDToSocketAddress(9151314447111828169));
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

void throwAwayCallback(PDUPeerEventPtr)
{
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest,
       StartAndShutdownMustBeCalledAsAPair_CallbackMustBeSetBeforeStart)
{
    FTRACE;
    SocketAddress listenAddress(make_pair("127.0.0.1", 13001));

    SocketAddressVector addresses;
    addresses.push_back(listenAddress);
    addresses.push_back(make_pair("127.0.0.1", 13002));
    addresses.push_back(make_pair("127.0.0.1", 13003));

    PDUPeerSetBuilderImpl peerSet(
        listenAddress,
        addresses,
        throwAwayCallback);

    peerSet.Start();
    peerSet.Shutdown();
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

    mTestPeers[1]->DeletePeerSet();

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
        EXPECT_EQ(3, peer->GetConnectedEventCount());
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, PeersReceiveDisconnectEvent)
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

    mTestPeers[1]->DeletePeerSet();
    sleep(2);

    EXPECT_EQ(1, mTestPeers[0]->GetDisconnectedEventCount());
    EXPECT_EQ(1, mTestPeers[2]->GetDisconnectedEventCount());
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, CanBroadcastPDUWithOptionalData)
{
    FTRACE;
    setupThreePeers();
    waitForAllConnected();

    PDUPtr pdu = makePDUWithOptionDataPtr();
    mTestPeers[0]->mPeerSet->BroadcastAsync(pdu);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetReceivedCount() == 0)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    PDUPtr expected = makePDUWithOptionDataPtr();
    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        EXPECT_EQ(1, peer->GetReceivedCount());
        EXPECT_EQ(1, peer->mReceivedPDUList.size());
        if (peer->mReceivedPDUList.size() == 1)
        {
            expectEqual(expected, peer->mReceivedPDUList.front());
        }
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, CanBroadcast2PDUsWithOptionalData)
{
    FTRACE;
    setupThreePeers();
    waitForAllConnected();

    PDUPtr pdu = makePDUWithOptionDataPtr();
    mTestPeers[0]->mPeerSet->BroadcastAsync(pdu);

    PDUPtr pdu2 = makePDUWithOptionDataPtr();
    mTestPeers[0]->mPeerSet->BroadcastAsync(pdu2);


    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetReceivedCount() < 2)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    PDUPtr expected = makePDUWithOptionDataPtr();
    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        ASSERT_EQ(2, peer->GetReceivedCount());
        ASSERT_EQ(2, peer->mReceivedPDUList.size());
        expectEqual(expected, peer->mReceivedPDUList.front());
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, AllPeersReceiveABroadcastPDUFromFirst)
{
    FTRACE;
    setupThreePeers();
    waitForAllConnected();

    PDUPtr pdu = makePDUPtr();
    mTestPeers[0]->mPeerSet->BroadcastAsync(pdu);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetReceivedCount() == 0)
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
        ASSERT_EQ(1, peer->GetReceivedCount());
        ASSERT_EQ(1, peer->mReceivedPDUList.size());
        expectEqual(expected, peer->mReceivedPDUList.front());
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, AllPeersReceiveABroadcastPDUFromMiddle)
{
    FTRACE;
    setupThreePeers();
    waitForAllConnected();

    PDUPtr pdu = makePDUPtr();
    mTestPeers[1]->mPeerSet->BroadcastAsync(pdu);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetReceivedCount() == 0)
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
        ASSERT_EQ(1, peer->GetReceivedCount());
        ASSERT_EQ(1, peer->mReceivedPDUList.size());
        expectEqual(expected, peer->mReceivedPDUList.front());
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, AllPeersReceiveABroadcastPDUFromLast)
{
    FTRACE;
    setupThreePeers();
    waitForAllConnected();

    PDUPtr pdu = makePDUPtr();
    mTestPeers[2]->mPeerSet->BroadcastAsync(pdu);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetReceivedCount() == 0)
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
        ASSERT_EQ(1, peer->GetReceivedCount());
        ASSERT_EQ(1, peer->mReceivedPDUList.size());
        expectEqual(expected, peer->mReceivedPDUList.front());
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, StopsReceivingEventsIfCallbackIsDisabled)
{
    FTRACE;
    setupThreePeers();
    waitForAllConnected();
    mTestPeers[1]->mPeerSet->SetEventCallback(NULL);

    PDUPtr pdu = makePDUPtr();
    mTestPeers[0]->mPeerSet->BroadcastAsync(pdu);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        if (mTestPeers[0]->GetReceivedCount() > 0
            && mTestPeers[2]->GetReceivedCount() > 0)
        {
            callbacksDone = false;
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    sleep(1);

    ASSERT_EQ(0, mTestPeers[1]->GetReceivedCount());
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest, CanBroadcastAtLeast50PDUsFromEach)
{
    FTRACE;
    setupThreePeers();
    waitForAllConnected();

    for (int i=0; i<50; i++)
    {
        PDUPtr pdu = makePDUPtr(i);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            peer->mPeerSet->BroadcastAsync(pdu);
        }
    }

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(60);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetReceivedCount() < 150)
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
        //EXPECT_EQ(0, peer->mPeerSet->GetQueueSize());
        EXPECT_EQ(150, peer->GetReceivedCount());
        EXPECT_EQ(150, peer->mReceivedPDUList.size());

        while (!peer->mReceivedPDUList.empty())
        {
            PDUPtr expected =
                makePDUPtr(peer->mReceivedPDUList.front()->GetOpcode());
            expectEqual(expected, peer->mReceivedPDUList.front());
            peer->mReceivedPDUList.pop_front();
        }
    }
}


/* changing the assumption that PDUPeerSet will retry PDUs for down
 * peers. It will still reconnect, but the resends seem to be doing
 * more harm than good
TEST_F(PDUPeerSetBuilderImplOnBoxTest,
       PDUWillComeAcrossIfConnectorDiesAndRestarts)
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
        if (mTestPeers[1]->GetReceivedCount() < 1)
        {
            callbacksDone = false;
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    ASSERT_EQ(1, mTestPeers[1]->GetReceivedCount());
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
        if (mTestPeers[0]->GetReceivedCount() < 1)
        {
            callbacksDone = false;
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    ASSERT_EQ(1, mTestPeers[0]->GetReceivedCount());
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
        if (mTestPeers[0]->GetReceivedCount() < 100)
        {
            callbacksDone = false;
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    ASSERT_EQ(100, mTestPeers[0]->GetReceivedCount());
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
            if (peer->GetReceivedCount() < 3)
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
            if (peer->GetReceivedCount() < 100)
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
        ASSERT_EQ(100, peer->GetReceivedCount());
        ASSERT_EQ(100, peer->mReceivedPDUList.size());

        while (!peer->mReceivedPDUList.empty())
        {
            PDUPtr expected = makePDUPtr(peer->mReceivedPDUList.front()->GetOpcode());
            expectEqual(expected, peer->mReceivedPDUList.front());
            peer->mReceivedPDUList.pop_front();
        }
    }
}
*/

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
            if (peer->GetReceivedCount() < 3000)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        sleep(1);
    }

    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        ASSERT_EQ(3000, peer->GetReceivedCount());
        ASSERT_EQ(3000, peer->mReceivedPDUList.size());

        while (!peer->mReceivedPDUList.empty())
        {
            PDUPtr expected = makePDUPtr(peer->mReceivedPDUList.front()->GetOpcode());
            expectEqual(expected, peer->mReceivedPDUList.front());
            peer->mReceivedPDUList.pop_front();
        }
    }
}

TEST_F(PDUPeerSetBuilderImplOnBoxTest,
       CanBroadcastAndReceive10000PDUs)
{
    FTRACE;
    setupThreePeers();

    for (int i=0; i<10000; i++)
    {
        PDUPtr pdu = makePDUWithOptionDataPtr(3, 1024);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            peer->mPeerSet->BroadcastAsync(pdu);
        }
    }

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(600);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetReceivedCount() < 30000)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        sleep(1);
    }

    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        ASSERT_EQ(30000, peer->GetReceivedCount());
        ASSERT_EQ(30000, peer->mReceivedPDUList.size());

        while (!peer->mReceivedPDUList.empty())
        {
            PDUPtr expected = makePDUWithOptionDataPtr(3, 1024);
            expectEqual(expected, peer->mReceivedPDUList.front());
            peer->mReceivedPDUList.pop_front();
        }
    }
}

//NOTE: PDUs with optional data larger than receiveBufferMaxSize in
//PDUPeerFileDescriptorEndpoint endpoint will lock up
TEST_F(PDUPeerSetBuilderImplOnBoxTest, CanBroadcastPDUWithHalfMegOptionalData)
{
    FTRACE;
    setupTwoPeers();
    waitForAllConnected();

    PDUPtr pdu = makePDUWithOptionDataPtr(3, 512*1024);
    mTestPeers[0]->mPeerSet->BroadcastAsync(pdu);

    DeadlineClock deadline;
    deadline.ExpiresInSeconds(10);
    while (!deadline.Expired())
    {
        bool callbacksDone(true);
        foreach(const TestPeerPtr& peer, mTestPeers)
        {
            if (peer->GetReceivedCount() == 0)
            {
                callbacksDone = false;
            }
        }

        if (callbacksDone)
            break;

        usleep(10000);
    }

    PDUPtr expected = makePDUWithOptionDataPtr(3, 512*1024);
    foreach(const TestPeerPtr& peer, mTestPeers)
    {
        EXPECT_EQ(1, peer->GetReceivedCount());
        EXPECT_EQ(1, peer->mReceivedPDUList.size());
        if (peer->mReceivedPDUList.size() == 1)
        {
            expectEqual(expected, peer->mReceivedPDUList.front());
        }
    }
}

//This is purely to ensure PDUPeers can teardown and setup ok
//underload, no checking of PDUs sent/recv'd will be done
TEST_F(PDUPeerSetBuilderImplOnBoxTest,
       PDUPeersCanDieAndRecoverUnderLoad)
{
    FTRACE;
    setupPeers(8);

    const TestPeerPtr& broadcaster(mTestPeers[0]);
    const TestPeerPtr& flaker(mTestPeers[1]);
    int i = 0;
    DeadlineClock deadline;
    deadline.ExpiresInSeconds(600);
    while (!deadline.Expired())
    {
        i++;
        PDUPtr pdu = makePDUPtr(i);

        if (hlog_ratelimit(10))
        {
            for (int j=1; j<8; j++)
                hlogstream(HLOG_INFO,
                           "queue size of " << j << ": "
                           << broadcaster->mPeerSet
                           ->GetPeer(mTestPeers[j]->mPeerID)->GetQueueSize());
        }


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
