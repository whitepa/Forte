#define BOOST_TEST_MODULE "PDUPeer Unit Tests"

#include "boost/test/unit_test.hpp"
#include <boost/shared_ptr.hpp>

#include "LogManager.h"
#include "Context.h"
#include "Foreach.h"
#include "PDUPeer.h"

using namespace boost::unit_test;

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

BOOST_AUTO_TEST_CASE(receivePDUTest)
{
    receivePDU();
}
BOOST_AUTO_TEST_CASE(receiveMultiplePDUTest)
{
    receiveMultiplePDU();
}
