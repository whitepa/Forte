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
#include "PDUPeerNetworkAcceptorEndpoint.h"

#include "ReceiverThread.h"
#include "OnDemandDispatcher.h"
#include "ThreadPoolDispatcher.h"
#include "AutoMutex.h"

using namespace Forte;

using ::testing::_;

Forte::LogManager logManager;

class PDUPeerSetOnBoxTest : public ::testing::Test
{
public:
    PDUPeerSetOnBoxTest()
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
    }

    void SetUp() {
        mConnectCount = 0;
        mReceivedPDUCount = 0;
    }

    void TearDown() {
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
};

TEST_F(PDUPeerSetOnBoxTest, ConstructDestruct)
{
    Forte::ThreadPoolDispatcherPtr workDispatcher;
    std::vector<boost::shared_ptr<PDUPeer> > peers;
    PDUPeerSetImpl peerSet(peers);
}

class SenderThread : public Forte::Thread
{
public:
    SenderThread(const Forte::PDUPeerEndpointPtr& endpoint)
        : mEndpoint(endpoint),
          mComplete(false)
        {
            initialized();
        }

    ~SenderThread() {
        deleting();
    }

    bool IsComplete() {
        return mComplete;
    }

protected:
    void* run() {
        // fill acceptor outgoing queue
        hlog(HLOG_INFO, "Start sending PDUs");

        // get tcp buffer size
        int tcpBufferSize = 0;
        socklen_t tcpBufferSizeLength = sizeof(tcpBufferSize);
        int err = getsockopt(mEndpoint->GetFD(),
                             SOL_SOCKET,
                             SO_SNDBUF,
                             &tcpBufferSize,
                             &tcpBufferSizeLength);


        int bufferSize;
        if (err == -1)
        {
            hlogstream(HLOG_INFO, "couldn't get bufsize, errno: " << errno);
            tcpBufferSize = 124928;
        }
        else
        {
            hlogstream(HLOG_INFO,
                       "will fill tcp buffer at size: " << tcpBufferSize);
        }

        bufferSize = tcpBufferSize - 1;
        hlogstream(HLOG_INFO, "will send PDUs of size " << bufferSize);

        boost::shared_array<char> buffer(new char[bufferSize]);
        Forte::PDUPtr pdu(new PDU(0, bufferSize, buffer.get()));

        for (int i=0; i<10; i++)
        {
            hlog(HLOG_INFO, "SendPDU");
            mEndpoint->SendPDU(*pdu);
        }

        mComplete = true;
        return NULL;
    }

protected:
    Forte::PDUPeerEndpointPtr mEndpoint;
    bool mComplete;
};

TEST_F(PDUPeerSetOnBoxTest, CanReceiveDataWhenSendQueueIsFull)
{
    // create two PDUPeerSets with 1 peer each
    //
    // StartPolling on the first one
    //
    // also on the first, enqueue large PDUs until the tcp send queue
    // is full. they will not be getting recv'd by the second since
    // StartPolling has not been called.
    //
    // send some PDUs to the first
    //
    // verify the PDUReady callback is fired

    std::vector<SocketAddress> socketAddresses;
    socketAddresses.push_back(std::make_pair("127.0.0.1", 15001));
    socketAddresses.push_back(std::make_pair("127.0.0.1", 15002));

    // connector setup
    PDUPeerNetworkConnectorEndpointPtr connectorEndpoint(
        new PDUPeerNetworkConnectorEndpoint(0, socketAddresses[1]));
    PDUPeerPtr connectorPeer(new PDUPeerImpl(0, connectorEndpoint));
    std::vector<boost::shared_ptr<PDUPeer> > connectorPeers;
    connectorPeers.push_back(connectorPeer);
    boost::shared_ptr<PDUPeerSetImpl> connectorPeerSet(
        new PDUPeerSetImpl(
            connectorPeers));
    // end connector setup

    // acceptor setup
    PDUPeerNetworkAcceptorEndpointPtr acceptorEndpoint(
        new PDUPeerNetworkAcceptorEndpoint(socketAddresses[0]));
    PDUPeerPtr acceptorPeer(new PDUPeerImpl(0, acceptorEndpoint));
    std::vector<boost::shared_ptr<PDUPeer> > acceptorPeers;
    acceptorPeers.push_back(acceptorPeer);
    boost::shared_ptr<PDUPeerSetImpl> acceptorPeerSet(
        new PDUPeerSetImpl(acceptorPeers));

    acceptorPeerSet->SetupEPoll();
    acceptorPeerSet->SetEventCallback(
        boost::bind(&PDUPeerSetOnBoxTest::EventCallback, this, _1));

    // begin acceptor incoming connection setup
    boost::shared_ptr<PDUPeerSetConnectionHandler> connectionHandler(
        new PDUPeerSetConnectionHandler(acceptorPeerSet));

    boost::shared_ptr<OnDemandDispatcher> connectionDispatcher(
        new OnDemandDispatcher(
            connectionHandler,
            1, // max threads
            32, // deepQueue - appears to be unused here also
            32, // maxDepth
            "PDUConnectionHandler")
        );

    ReceiverThread receiverThread(
        connectionDispatcher,
        "PDU",
        15002,
        32, // backlog
        "127.0.0.1");
    // end incoming connections

    acceptorPeerSet->StartPolling();
    connectorPeerSet->StartPolling();

    {
        AutoUnlockMutex lock(mConnectMutex);
        while (mConnectCount == 0)
        {
            mConnectCondition.Wait();
        }
    }

    ASSERT_EQ(1, mConnectCount);

    hlog(HLOG_INFO, "Connected");
    SenderThread thread(acceptorEndpoint);

    sleep(1);
    ASSERT_FALSE(thread.IsComplete());

    //may need a better way to definitively say that we are blocked on
    //send(). not sure how to do that, experimentally it is true
    boost::shared_array<char> buffer(new char[1024]);
    Forte::PDUPtr pdu(new PDU(0, 1024, buffer.get()));

    hlog(HLOG_INFO, "SendPDU from connector");
    connectorEndpoint->SendPDU(*pdu);

    {
        AutoUnlockMutex lock(mReceivedMutex);
        while (mReceivedPDUCount == 0)
        {
            hlog(HLOG_INFO, "Waiting to recv PDU");
            mReceivedCondition.Wait();
        }
    }

    ASSERT_EQ(1, mReceivedPDUCount);

    hlog(HLOG_INFO, "now clear out q and let acceptor finish");
    connectorPeerSet->SetEventCallback(
        boost::bind(&PDUPeerSetOnBoxTest::EventCallback, this, _1));
    connectorPeerSet->SetupEPoll();
    connectorPeerSet->StartPolling();
}


/*class AsyncDataLoaderThread : public Forte::Thread
{
public:
    AsyncDataLoaderThread(Forte::PDUPeerSet& peer)
        : mPeer(peer),
          mReceiveCount(0)
        {
            initialized();
        }

    ~AsyncDataLoaderThread() {
        deleting();
    }

    int GetReceiveCount() {
        AutoUnlockMutex lock(mReceiveCountMutex);
        return mReceiveCount;
    }

protected:
    void* run() {
        Forte::PDU pdu;
        size_t len;
        shared_array<char> buf = makeTestPDU(pdu, len);

        for (int i = 0; i < 100; ++i)
        {
            mPeer.DataIn(len, buf.get());
            {
                AutoUnlockMutex lock(mReceiveCountMutex);
                mReceiveCount++;
            }
        }
        return NULL;
    }

protected:
    Forte::PDUPeerSet& mPeer;
    Forte::Mutex mReceiveCountMutex;
    int mReceiveCount;
};
*/

/*TEST_F(PDUPeerSetOnBoxTest, ThrowESendFailedOnBrokenPipe)
{
    FTRACE;

    try
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        Forte::AutoFD fd1(fds[0]);
        Forte::AutoFD fd2(fds[1]);

        AutoFD epollFD;
        epollFD = epoll_create(4);
        Forte::PDUPeerSet endpoint1(fd1);
        Forte::PDUPeerSet endpoint2(fd2);

        endpoint1.SetEPollFD(epollFD);
        endpoint2.SetEPollFD(epollFD);

        PDU pdu, rpdu;
        size_t len;
        makeTestPDU(pdu, len);

        endpoint1.SendPDU(pdu);

        // poll for events
        int fdReadyCount = 0;
        struct epoll_event events[32];
        memset(events, 0, 32*sizeof(struct epoll_event));
        fdReadyCount = epoll_wait(epollFD, events, 32, 1000);
        EXPECT_EQ(1, fdReadyCount);
        ASSERT_EQ((int) fd2, events[0].data.fd);
        ASSERT_EQ(endpoint2.GetFD(), events[0].data.fd);

        endpoint2.HandleEPollEvent(events[0]);

        EXPECT_EQ(true, endpoint2.IsPDUReady());
        EXPECT_EQ(true, endpoint2.RecvPDU(rpdu));
        EXPECT_EQ(true, pdu == rpdu);

        close(fds[1]);

        ASSERT_THROW(endpoint1.SendPDU(pdu), EPeerSendFailed);

    }
    catch (Forte::Exception &e)
    {
        hlog(HLOG_ERR, "caught exception: %s\n", e.what());
        FAIL();
    }
}
*/
