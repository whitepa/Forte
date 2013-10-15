#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "FTrace.h"
#include "LogManager.h"

#include "Murmur.h"

using namespace std;
using namespace boost;
using namespace Forte;

using ::testing::_;
using ::testing::Return;
using ::testing::UnitTest;

LogManager logManager;

class MurmurUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
    }

    static void TearDownTestCase() {
        logManager.EndLogging();
    }

    void SetUp() {
        hlogstream(
            HLOG_INFO, "Starting test "
            << UnitTest::GetInstance()->current_test_info()->name());
    }

    void TearDown() {
        hlogstream(
            HLOG_INFO, "ending test "
            << UnitTest::GetInstance()->current_test_info()->name());
    }


    FString test(const FString& data, size_t buf_size) {
        Murmur64 m;
        const char *cdata = data.c_str();
        size_t len, size = data.length();

        while (size)
        {
            // get maximum update size
            len = min<size_t>(size, buf_size);

            // update hash
            m.Update(cdata, len);

            // update pointer and remaining size
            cdata += len;
            size -= len;
        }

        m.Final();
        return m;
    }


    FString rdata(size_t size) {
        FString stmp, data;
        size_t i, r, n = (size / 2);

        // create some random data, 2 bytes at a time
        for (i=0; i<n; i++)
        {
            r = (rand() >> 2) & 0xFFFF;
            stmp.assign(reinterpret_cast<char*>(&r), 2);
            data += stmp;
        }

        // one more byte?
        if (size % 2)
        {
            r = (rand() >> 5);
            stmp.assign(r, 1);
            data += stmp;
        }

        return data;
    }
};


TEST_F(MurmurUnitTest, LegacyTest)
{
    FString stmp, h1, h2, data;
    // init
    srand(time(NULL));

    // test a few different buffer sizes
    data = rdata(256);
    h1 = test(data, 128);
    h2 = test(data, 256);
    EXPECT_EQ(h1, h2) << "hash1 [128] == hash1 [256]";

    h1 = test(data, 100);
    EXPECT_EQ(h1, h2) << "hash1 [100] == hash1 [256]";

    h1 = test(data, 8);
    EXPECT_EQ(h1, h2) << "hash1 [8] == hash1 [256]";

    // test empty
    data.clear();
    h1 = test(data, 128);
    h2 = test(data, 256);
    EXPECT_EQ(h1, h2) << "empty [128] == empty [256]";

    h1 = test(data, 0);
    EXPECT_EQ(h1, h2) << "empty [0] == empty [256]";

    // test exception
    data = rdata(1024);
    EXPECT_ANY_THROW(test(data, 1));
    h1 = test(data, 2048);
    h2 = test(data, 1024);
    EXPECT_EQ(h1, h2) << "hash2 [2048] == hash2 [1024]";

    // test small data with different buffer sizes
    data = rdata(8);
    h1 = test(data, 7);
    h2 = test(data, 8);
    EXPECT_EQ(h1, h2) << "hash3 [7] == hash3 [8]";

    EXPECT_ANY_THROW(test(data, 3))
        << "expected FORTE_MURMUR_TOO_SMALL, hash3 [3] exception";
    h1 = test(data, 4);
    EXPECT_EQ(h1, h2) << "hash3 [4] == hash3 [8]";

}
