
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "AutoFD.h"
#include "FTrace.h"
#include "LogManager.h"
#include "Context.h"
#include "Foreach.h"
#include "PDUPeer.h"
#include "PDUPeerSet.h"

using namespace boost;
using namespace Forte;

LogManager logManager;
// -----------------------------------------------------------------------------

class PDUPeerUnitTest : public ::testing::Test
{
public:
    // Override this to define how to set up the environment.
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");
        hlog(HLOG_DEBUG, "Starting test...");
    }

    // Override this to define how to tear down the environment.
    static void TearDownTestCase() {
    }

};
// -----------------------------------------------------------------------------

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

TEST_F(PDUPeerUnitTest, receivePDUTest)
{
    FTRACE;

    Forte::PDU pdu;
    size_t len;
    makeTestPDU(pdu, len);

    Forte::PDUPeer peer(0);
    peer.DataIn(len, (char *)&pdu);

    Forte::PDU out;
    bool received = peer.RecvPDU(out);
    ASSERT_EQ(received, true);

    int cmp = memcmp(&pdu, &out, len);
    ASSERT_EQ(cmp, 0);
}

TEST_F(PDUPeerUnitTest, receiveMultiplePDUTest)
{
    FTRACE;

    Forte::PDU pdu;
    size_t len;
    makeTestPDU(pdu, len);

    Forte::PDUPeer peer(0);

    for (int i = 0; i < 3; ++i)
    {
        peer.DataIn(len, (char *)&pdu);
    }

    Forte::PDU out;
    for (int i = 0; i < 3; ++i)
    {
        bool received = peer.RecvPDU(out);
        ASSERT_EQ(received, true);
        int cmp = memcmp(&pdu, &out, len);
        ASSERT_EQ(cmp, 0);
    }
}

TEST_F(PDUPeerUnitTest, pollPDUTest)
{
    FTRACE;

    try
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        Forte::AutoFD fd1(fds[0]);
        Forte::AutoFD fd2(fds[1]);
        Forte::PDUPeerSet set1;
        Forte::PDUPeerSet set2;

        ASSERT_THROW(set1.Poll(), EPDUPeerSetNotPolling);

        shared_ptr<PDUPeer> peer1 = set1.PeerCreate(fd1);

        set1.SetupEPoll();
        set2.SetupEPoll();

        shared_ptr<PDUPeer> peer2 = set2.PeerCreate(fd2);

        PDU pdu, rpdu;
        size_t len;
        makeTestPDU(pdu, len);

        peer1->SendPDU(pdu);

        set2.Poll(1000);

        EXPECT_EQ(true, peer2->IsPDUReady());
        EXPECT_EQ(true, peer2->RecvPDU(rpdu));
        EXPECT_EQ(0, memcmp(&pdu, &rpdu, sizeof(PDU)));
    }
    catch (Forte::Exception &e)
    {
        hlog(HLOG_ERR, "caught exception: %s\n", e.what());
    }
}

TEST_F(PDUPeerUnitTest, DataInBufferOverflowTest)
{
    FTRACE;

    Forte::PDU pdu;
    size_t len;
    makeTestPDU(pdu, len);

    Forte::PDUPeer peer(0, 512);

    for (int i = 0; i < 3; ++i)
    {
        peer.DataIn(len, (char *)&pdu);
    }

    ASSERT_THROW(peer.DataIn(len, (char*) &pdu), EPeerBufferOverflow);
}


TEST_F(PDUPeerUnitTest, ThrowESendFailedOnBrokenPipe)
{
    FTRACE;

    try
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        Forte::AutoFD fd1(fds[0]);
        Forte::AutoFD fd2(fds[1]);
        Forte::PDUPeerSet set1;
        Forte::PDUPeerSet set2;

        shared_ptr<PDUPeer> peer1 = set1.PeerCreate(fd1);

        set1.SetupEPoll();
        set2.SetupEPoll();

        shared_ptr<PDUPeer> peer2 = set2.PeerCreate(fd2);

        PDU pdu, rpdu;
        size_t len;
        makeTestPDU(pdu, len);

        peer1->SendPDU(pdu);

        set2.Poll(1000);

        EXPECT_EQ(true, peer2->IsPDUReady());
        EXPECT_EQ(true, peer2->RecvPDU(rpdu));
        EXPECT_EQ(0, memcmp(&pdu, &rpdu, sizeof(PDU)));

   close(fds[1]);
        
   ASSERT_THROW(peer1->SendPDU(pdu), EPeerSendFailed);

    }
    catch (Forte::Exception &e)
    {
        hlog(HLOG_ERR, "caught exception: %s\n", e.what());
    }
}

