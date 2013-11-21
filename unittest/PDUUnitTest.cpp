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
    Opcode2Payload()
        : int64Data(0),
          int32Data(0)
        {
            memset(smallCString, 0, 255);
        }

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

TEST_F(PDUUnitTest, ConstructWithPayloadSetOptionalData)
{
    FTRACE;

    Opcode2Payload payload;
    size_t payloadSize = sizeof(payload);
    payload.int64Data = 10000;
    payload.int32Data = 100;
    strcpy(payload.smallCString, "test data 1");

    size_t optionalDataSize = 100;

    PDU pdu(Opcode2, payloadSize, &payload);

    boost::shared_ptr<PDUOptionalData> data(
        new PDUOptionalData(optionalDataSize, 0));
    memset(data->mData, 1, optionalDataSize);

    pdu.SetOptionalData(data);

    EXPECT_EQ(Opcode2, pdu.GetOpcode());
    EXPECT_EQ(payloadSize, pdu.GetPayloadSize());
    EXPECT_EQ(payload, *(pdu.GetPayload<Opcode2Payload>()));
    EXPECT_EQ(optionalDataSize, pdu.GetOptionalDataSize());
    EXPECT_TRUE(memcmp(data->mData,
                       pdu.GetOptionalData()->GetData(),
                       optionalDataSize) == 0);
}

TEST_F(PDUUnitTest, CanRequestMemAlignedOptionalData)
{
    FTRACE;

    size_t optionalDataSize = 100;
    boost::shared_ptr<PDUOptionalData> data(
        new PDUOptionalData(optionalDataSize,
                            PDU_OPTIONAL_DATA_ATTRIBUTE_MEMALIGN_512));
    memset(data->mData, 1, optionalDataSize);

    PDU pdu(Opcode3, 0, NULL);
    pdu.SetOptionalData(data);

    EXPECT_EQ(Opcode3, pdu.GetOpcode());
    EXPECT_EQ(0, pdu.GetPayloadSize());
    EXPECT_EQ(NULL, pdu.GetPayload<Opcode2Payload>());
    EXPECT_EQ(optionalDataSize, pdu.GetOptionalDataSize());
    EXPECT_TRUE(memcmp(data->mData,
                       pdu.GetOptionalData()->GetData(), optionalDataSize) == 0);
    EXPECT_EQ(0, (reinterpret_cast<unsigned long long>(pdu.GetOptionalData()->GetData())) % 512);
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
    memset(payload.smallCString, 1, 255);
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

TEST_F(PDUUnitTest, CompareWithPayloadAndOptionalData)
{
    FTRACE;

    Opcode2Payload payload;
    size_t payloadSize = sizeof(payload);
    payload.int64Data = 10000;
    payload.int32Data = 100;
    memset(payload.smallCString, 0, sizeof(payload.smallCString));
    strcpy(payload.smallCString, "test data 1");

    size_t optionalDataSize = 100;
    boost::shared_ptr<PDUOptionalData> data(
        new PDUOptionalData(optionalDataSize,
                            PDU_OPTIONAL_DATA_ATTRIBUTE_MEMALIGN_512));
    memset(data->mData, 1, optionalDataSize);


    PDU pdu1(Opcode2, payloadSize, &payload);
    pdu1.SetOptionalData(data);

    Opcode2Payload payload2;
    payload2.int64Data = 10000;
    payload2.int32Data = 100;
    memset(payload2.smallCString, 0, sizeof(payload.smallCString));
    strcpy(payload2.smallCString, "test data 1");

    boost::shared_ptr<PDUOptionalData> data2(
        new PDUOptionalData(optionalDataSize,
                            PDU_OPTIONAL_DATA_ATTRIBUTE_MEMALIGN_512));
    memset(data2->mData, 1, optionalDataSize);

    PDU pdu2(Opcode2, payloadSize, &payload2);
    pdu2.SetOptionalData(data2);

    EXPECT_EQ(pdu1, pdu2);
}

TEST_F(PDUUnitTest, ACopiedPDUIsEqualToTheOriginal)
{
    FTRACE;

    Opcode2Payload payload;
    size_t payloadSize = sizeof(payload);
    payload.int64Data = 10000;
    payload.int32Data = 100;
    memset(payload.smallCString, 0, sizeof(payload.smallCString));
    strcpy(payload.smallCString, "test data 1");

    size_t optionalDataSize = 100;
    boost::shared_ptr<PDUOptionalData> data(
        new PDUOptionalData(optionalDataSize,
                            PDU_OPTIONAL_DATA_ATTRIBUTE_MEMALIGN_512));
    memset(data->mData, 1, optionalDataSize);


    PDU pdu1(Opcode2, payloadSize, &payload);
    pdu1.SetOptionalData(data);

    PDU pdu2 = pdu1;

    EXPECT_EQ(pdu1, pdu2);
}

TEST_F(PDUUnitTest, IncompatiblePayloadVersions)
{
    PDU pdu1(Opcode0, 0, 0, 1);
    PDU pdu2 = pdu1;
    PDU pdu3(Opcode0, 0, 0, 3);

    ASSERT_TRUE(pdu1 == pdu2);

    pdu2.SetPayloadVersion(3);

    ASSERT_FALSE(pdu1 == pdu2);
    ASSERT_FALSE(pdu1 == pdu3);
    ASSERT_TRUE(pdu2 == pdu3);
}

/* a lot code depends on these semantics to work. will update that
 * code in the next pdu revision.

TEST_F(PDUUnitTest, PoorlyConstructedPDUsDoNotSegfault)
{
    FTRACE;

    Opcode2Payload payload;
    size_t payloadSize = sizeof(payload);
    payload.int64Data = 10000;
    payload.int32Data = 100;
    memset(payload.smallCString, 0, sizeof(payload.smallCString));
    strcpy(payload.smallCString, "test data 1");

    size_t optionalDataSize = 100;
    scoped_array<char> buf(new char[optionalDataSize]);
    memset(buf.get(), 1, optionalDataSize);

    PDU pdu1(Opcode2, payloadSize, &payload, optionalDataSize, buf.get());

    PDU pdu2;
    EXPECT_FALSE(pdu1 == pdu2);

    EXPECT_ANY_THROW(PDU pdu3(Opcode2, 10000, NULL));
    //EXPECT_FALSE(pdu1 == pdu3);

    EXPECT_ANY_THROW(PDU pdu4(Opcode2, 10000, NULL, 400, NULL));
    //EXPECT_FALSE(pdu1 == pdu4);

    char buf20[20] = {0};
    EXPECT_ANY_THROW(PDU pdu5(Opcode2, 20, &buf20, 400, NULL));
    //EXPECT_FALSE(pdu1 == pdu5);

    // don't think we can protect against these, they are not
    // PoorlyConstructed, they are HorriblyConstructed

    //PDU pdu6(Opcode2, 10000, &buf20, 400, &buf20);
    //EXPECT_FALSE(pdu1 == pdu6);

    //PDU pdu7(Opcode2, 0, NULL, 400, &buf20);
    //EXPECT_FALSE(pdu1 == pdu7);

    PDU pdu8(Opcode2, 5, &buf20);
    EXPECT_FALSE(pdu1 == pdu8);

    PDU pdu9(Opcode2, 5, &buf20, 5, &buf20);
    EXPECT_FALSE(pdu1 == pdu9);
}
*/

