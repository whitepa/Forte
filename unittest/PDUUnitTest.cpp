#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "FTrace.h"
#include "LogManager.h"

#include "PDU.h"

using namespace std;
using namespace boost;
using namespace Forte;

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Throw;
using ::testing::Return;

LogManager logManager;

class PDUUnitTest : public ::testing::Test
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
    }

    void TearDown() {
    }
};

enum TestOpcode {
    Opcode0,
    Opcode1,
    Opcode2,
    Opcode3
};

struct Opcode2Payload {
    uint64_t int64Data;
    uint32_t int32Data;
    char smallCString[255];

    bool operator==(const Opcode2Payload& other) const {
        return
            int64Data == other.int64Data
            && int32Data == other.int32Data
            && (strncmp(smallCString, other.smallCString, 255) == 0);
    }
} __attribute__((__packed__));

std::ostream& operator<<(std::ostream& out, const Opcode2Payload& o)
{
    out << "{" << o.int64Data
        << "," << o.int32Data
        << "," << o.smallCString
        << "}";
    return out;
}

TEST_F(PDUUnitTest, ConstructDestruct)
{
    FTRACE;
    PDU pdu;
}

TEST_F(PDUUnitTest, ConstructWithOpcde)
{
    FTRACE;

    PDU pdu(Opcode0);
    EXPECT_EQ(Opcode0, pdu.GetOpcode());
    EXPECT_EQ(0, pdu.GetPayloadSize());
    EXPECT_EQ(0, pdu.GetOptionalDataSize());
}

TEST_F(PDUUnitTest, ConstructWithGenericPayload)
{
    FTRACE;

    size_t payloadSize = 100;
    scoped_array<char> buf(new char[payloadSize]);
    memset(buf.get(), 0, payloadSize);

    PDU pdu(Opcode1, payloadSize, buf.get());
    EXPECT_EQ(Opcode1, pdu.GetOpcode());
    EXPECT_EQ(payloadSize, pdu.GetPayloadSize());
    EXPECT_STREQ(buf.get(), pdu.GetPayload<char>());
    EXPECT_EQ(0, pdu.GetOptionalDataSize());
}

TEST_F(PDUUnitTest, ConstructWithPackedStructPayload)
{
    FTRACE;

    Opcode2Payload payload;
    size_t payloadSize = sizeof(payload);
    payload.int64Data = 10000;
    payload.int32Data = 100;
    strcpy(payload.smallCString, "test data 1");

    PDU pdu(Opcode2, payloadSize, &payload);
    EXPECT_EQ(Opcode2, pdu.GetOpcode());
    EXPECT_EQ(payloadSize, pdu.GetPayloadSize());
    EXPECT_EQ(payload, *(pdu.GetPayload<Opcode2Payload>()));
    EXPECT_EQ(0, pdu.GetOptionalDataSize());
}

TEST_F(PDUUnitTest, CompareIdentity)
{
    FTRACE;
    PDU pdu;
    ASSERT_EQ(pdu, pdu);
}

TEST_F(PDUUnitTest, CompareWithOpcde)
{
    FTRACE;

    PDU pdu1(Opcode0);
    PDU pdu2(Opcode0);

    EXPECT_EQ(pdu1, pdu2);
}

TEST_F(PDUUnitTest, GenericPayloadComparesByValue)
{
    FTRACE;

    size_t payloadSize = 100;
    scoped_array<char> buf(new char[payloadSize]);
    memset(buf.get(), 1, payloadSize);

    PDU pdu1(Opcode1, payloadSize, buf.get());

    scoped_array<char> buf2(new char[payloadSize]);
    memset(buf2.get(), 1, payloadSize);

    PDU pdu2(Opcode1, payloadSize, buf2.get());

    EXPECT_EQ(pdu1, pdu2);
}

TEST_F(PDUUnitTest, PackedStructPayloadComparesDoesNotUseStructEqOperator)
{
    FTRACE;

    Opcode2Payload payload;
    size_t payloadSize = sizeof(payload);
    payload.int64Data = 10000;
    payload.int32Data = 100;
    strcpy(payload.smallCString, "test data 1");

    PDU pdu1(Opcode2, payloadSize, &payload);

    Opcode2Payload payload2;
    payload2.int64Data = 10000;
    payload2.int32Data = 100;
    strcpy(payload2.smallCString, "test data 1");
    PDU pdu2(Opcode2, payloadSize, &payload2);

    EXPECT_FALSE(pdu1 == pdu2);
}

TEST_F(PDUUnitTest, PackedStructPayloadComparesByMemory)
{
    FTRACE;

    Opcode2Payload payload;
    size_t payloadSize = sizeof(payload);
    payload.int64Data = 10000;
    payload.int32Data = 100;
    memset(payload.smallCString, 0, sizeof(payload.smallCString));
    strcpy(payload.smallCString, "test data 1");

    PDU pdu1(Opcode2, payloadSize, &payload);

    Opcode2Payload payload2;
    payload2.int64Data = 10000;
    payload2.int32Data = 100;
    memset(payload2.smallCString, 0, sizeof(payload.smallCString));
    strcpy(payload2.smallCString, "test data 1");
    PDU pdu2(Opcode2, payloadSize, &payload2);

    EXPECT_EQ(pdu1, pdu2);
}
