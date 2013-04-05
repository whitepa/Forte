#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"
#include "CRC32COptimal.h"

using namespace std;
using namespace boost;
using namespace Forte;

LogManager logManager;

class CRCTest : public ::testing::Test
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

TEST_F(CRCTest, CRC32COptimal_for_each)
{
    FTRACE;

    std::vector<unsigned char> testData;
    for(unsigned long i='0'; i<'8'; i++)
        testData.push_back(i);

    EXPECT_EQ(std::for_each(testData.begin(), testData.end(), CRC32COptimal<>()).GetChecksum(), 0x200a91aa);
}

TEST_F(CRCTest, CRC32COptimal_64Bit)
{
    FTRACE;
    unsigned char const testData[] = { '0', '1', '2', '3', '4', '5', '6', '7'};
    std::size_t const testDataLen = sizeof(testData) / sizeof(testData[0]);

    CRC32COptimal<> crc;
    crc.ProcessBytes(testData, testDataLen);
    EXPECT_EQ(0x200a91aa, crc.GetChecksum());
}

TEST_F(CRCTest, CRC32COptimal_32Bit)
{
    FTRACE;
    unsigned char const testData[] = { '0', '1', '2', '3', '4', '5', '6', '7'};
    std::size_t const testDataLen = sizeof(testData) / sizeof(testData[0]);

    CRC32COptimal<> crc;

    for(unsigned long i=0; i<testDataLen/sizeof(unsigned int); i++)
        crc.ProcessBytes(testData+i*sizeof(unsigned int), sizeof(unsigned int));

    EXPECT_EQ(0x200a91aa, crc.GetChecksum());
}

TEST_F(CRCTest, CRC32COptimal_16Bit)
{
    FTRACE;
    unsigned char const testData[] = { '0', '1', '2', '3', '4', '5', '6', '7'};
    std::size_t const testDataLen = sizeof(testData) / sizeof(testData[0]);

    CRC32COptimal<> crc;

    for(unsigned long i=0; i<testDataLen/sizeof(unsigned short); i++)
        crc.ProcessBytes(testData+i*sizeof(unsigned short), sizeof(unsigned short));

    EXPECT_EQ(0x200a91aa, crc.GetChecksum());
}

TEST_F(CRCTest, CRC32COptimal_8Bit)
{
    FTRACE;
    unsigned char const testData[] = { '0', '1', '2', '3', '4', '5', '6', '7'};
    std::size_t const testDataLen = sizeof(testData) / sizeof(testData[0]);

    CRC32COptimal<> crc;

    for(unsigned long i=0; i<testDataLen/sizeof(unsigned char); i++)
        crc.ProcessBytes(testData+i*sizeof(unsigned char), sizeof(unsigned char));

    EXPECT_EQ(0x200a91aa, crc.GetChecksum());
}

TEST_F(CRCTest, CRC32COptimal_Not_Power_Two)
{
    FTRACE;
    unsigned char const testData[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd'};
    std::size_t const testDataLen = sizeof(testData) / sizeof(testData[0]);

    CRC32COptimal<> crc;
    crc.ProcessBytes(testData, testDataLen);
    EXPECT_EQ(0xC52DB634, crc.GetChecksum());
}

typedef CRCBoost<CRC32COptimal<>::BITS,
                 CRC32COptimal<>::POLYNOMIAL,
                 CRC32COptimal<>::INITIAL,
                 CRC32COptimal<>::FINAL_XOR,
                 CRC32COptimal<>::REVERSE_DATA,
                 CRC32COptimal<>::REVERSE_RESULT>
        crc32c_boost_type;

typedef CRC32CIntelSSE4<CRC32COptimal<>::INITIAL>
        crc32c_intel_type;

TEST_F(CRCTest, CRC32CBoost_Not_Power_Two)
{
    FTRACE;
    unsigned char const testData[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd'};
    std::size_t const testDataLen = sizeof(testData) / sizeof(testData[0]);

    crc32c_boost_type crc;
    crc.ProcessBytes(testData, testDataLen);
    EXPECT_EQ(0xC52DB634, crc.GetChecksum());
}

TEST_F(CRCTest, CRC32CCompareIntelVsBoost)
{
    FTRACE;
    unsigned char testData[4*1024]; // potential randomness is good
    std::size_t const testDataLen = sizeof(testData) / sizeof(testData[0]);

    crc32c_boost_type crcBoost;
    crc32c_intel_type crcIntel;

    crcBoost.ProcessBytes(testData, testDataLen);
    crcIntel.ProcessBytes(testData, testDataLen);

    EXPECT_EQ(crcBoost, crcIntel);
}
