#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "FTrace.h"
#include "LogManager.h"

#include "GUIDGenerator.h"

using namespace std;
using namespace boost;
using namespace Forte;

LogManager logManager;

class GUIDGeneratorUnitTest : public ::testing::Test
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

TEST_F(GUIDGeneratorUnitTest, ConstructDelete)
{
    FTRACE;
    GUIDGenerator theClass;
}

TEST_F(GUIDGeneratorUnitTest, CanGenerateGUIDOfUint8_tVector)
{
    FTRACE;

    GUIDGenerator gg;
    std::set<std::string> guids;
    unsigned int num = 100;
    unsigned int i = num;
    while (i--)
    {
        std::vector<uint8_t> vec;
        gg.GenerateGUID(vec);
        std::string guid(vec.begin(), vec.end());
        guids.insert(guid);
    }
    ASSERT_EQ(guids.size(), num);
}

TEST_F(GUIDGeneratorUnitTest, CanConvertGUIDOfUint8_tVectorToString)
{
    FTRACE;

    GUIDGenerator gg;

    std::vector<uint8_t> vec;
    for(uint8_t i=0; i<16; i++)
    {
        vec.push_back(i);
    }

    std::string converted;
    gg.ToString(converted, vec);
    //format: "01234567-89ab-cdef-0123-456789abcdef"
    ASSERT_EQ("00010203-0405-0607-0809-0a0b0c0d0e0f",
              converted);
}

TEST_F(GUIDGeneratorUnitTest, CanConvertGUIDOfUint8_tArrayToString)
{
    FTRACE;

    GUIDGenerator gg;

    uint8_t vec[16];
    for(uint8_t i=0; i<16; i++)
    {
        vec[i] = i;
    }

    std::string converted;
    gg.ToString(converted, vec);
    //format: "01234567-89ab-cdef-0123-456789abcdef"
    ASSERT_EQ("00010203-0405-0607-0809-0a0b0c0d0e0f",
              converted);
}

TEST_F(GUIDGeneratorUnitTest, CanGenerate1MillionGUIDs)
{
    FTRACE;

    hlog(HLOG_INFO, "GUIDGenerator, verifying 1M unique GUIDs");

    GUIDGenerator gg;
    std::set<FString> guids;
    unsigned int num = 1000000;
    unsigned int i = num;
    while (i--)
    {
        FString guid;
        gg.GenerateGUID(guid);
        guids.insert(guid);
    }
    ASSERT_EQ(guids.size(), num);
}
