//#define BOOST_TEST_MODULE "PDUPeer Unit Tests"

#include "boost/test/unit_test.hpp"
#include <boost/shared_ptr.hpp>

#include "AutoFD.h"
#include "LogManager.h"
#include "Context.h"
#include "Foreach.h"
#include "PDUPeer.h"
#include "PDUPeerSet.h"

using namespace boost::unit_test;
using namespace boost;
using namespace Forte;

LogManager logManager;

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

void receivePDU(void)
{
    Forte::PDU pdu;
    size_t len;
    makeTestPDU(pdu, len);

    Forte::PDUPeer peer(0);
    peer.DataIn(len, (char *)&pdu);
    
    Forte::PDU out;
    bool received = peer.RecvPDU(out);
    BOOST_CHECK_EQUAL(received, true);

    int cmp = memcmp(&pdu, &out, len);
    BOOST_CHECK_EQUAL(cmp, 0);
}

void receiveMultiplePDU(void)
{
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
        BOOST_CHECK_EQUAL(received, true);
        int cmp = memcmp(&pdu, &out, len);
        BOOST_CHECK_EQUAL(cmp, 0);
    }
}

void pollPDU()
{
    try
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        Forte::AutoFD fd1(fds[0]);
        Forte::AutoFD fd2(fds[1]);
        Forte::PDUPeerSet set1;
        Forte::PDUPeerSet set2;

        BOOST_CHECK_THROW(set1.Poll(), EPDUPeerSetNotPolling);

        shared_ptr<PDUPeer> peer1 = set1.PeerCreate(fd1);
        BOOST_CHECK(peer1);

        set1.SetupEPoll();
        set2.SetupEPoll();

        shared_ptr<PDUPeer> peer2 = set2.PeerCreate(fd2);
        BOOST_CHECK(peer2);

        PDU pdu, rpdu;
        size_t len;
        makeTestPDU(pdu, len);

        peer1->SendPDU(pdu);

        set2.Poll(1000);

        BOOST_CHECK(peer2->IsPDUReady());
        BOOST_CHECK(peer2->RecvPDU(rpdu));
        BOOST_CHECK(!memcmp(&pdu, &rpdu, sizeof(PDU)));
    }
    catch (Forte::Exception &e)
    {
        BOOST_FAIL(e.what());
    }
}

BOOST_AUTO_TEST_CASE(receivePDUTest)
{
    receivePDU();
}
BOOST_AUTO_TEST_CASE(receiveMultiplePDUTest)
{
    receiveMultiplePDU();
}

BOOST_AUTO_TEST_CASE(pollUnitTest)
{
    pollPDU();
}

////Boost Unit init function ///////////////////////////////////////////////////
test_suite*
init_unit_test_suite(int argc, char* argv[])
{

    logManager.SetLogMask("//stdout", HLOG_ALL);
    logManager.BeginLogging("//stdout");

    return 0;
}
////////////////////////////////////////////////////////////////////////////////
