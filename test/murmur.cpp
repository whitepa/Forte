#include "Forte.h"
#include "ansi_test.h"

// functions
FString test(const FString& data, size_t buf_size)
{
    Murmur64 m;
    const char *cdata = (const char*)(data);
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

    // done
    m.Final();
    return m;
}


FString rdata(size_t size)
{
    FString stmp, data;
    size_t i, r, n = (size / 2);

    // create some random data, 2 bytes at a time
    for (i=0; i<n; i++)
    {
        r = (rand() >> 2) & 0xFFFF;
        stmp.assign((char*)&r, 2);
        data += stmp;
    }

    // one more byte?
    if (size % 2)
    {
        r = (rand() >> 5);
        stmp.assign(r, 1);
        data += stmp;
    }

    // done
    return data;
}


// main
int main(int argc, char *argv[])
{
    FString stmp, h1, h2, data;

    // init
    srand(time(NULL));

    // test a few different buffer sizes
    data = rdata(256);
    TEST_NO_EXP(h1 = test(data, 128), "hash1 [128]");
    TEST_NO_EXP(h2 = test(data, 256), "hash1 [256]");
    TEST(h1, h2, "hash1 [128] == hash1 [256]");

    TEST_NO_EXP(h1 = test(data, 100), "hash1 [100]");
    TEST(h1, h2, "hash1 [100] == hash1 [256]");

    TEST_NO_EXP(h1 = test(data, 8), "hash1 [8]");
    TEST(h1, h2, "hash1 [8] == hash1 [256]");

    // test empty
    data.clear();
    TEST_NO_EXP(h1 = test(data, 128), "empty [128]");
    TEST_NO_EXP(h2 = test(data, 256), "empty [256]");
    TEST(h1, h2, "empty [128] == empty [256]");

    TEST_NO_EXP(h1 = test(data, 0), "empty [0]");
    TEST(h1, h2, "empty [0] == empty [256]");

    // test exception
    data = rdata(1024);
    TEST_EXP(h1 = test(data, 1), "FORTE_MURMUR_TOO_SMALL", "hash2 [1] exception");
    TEST_NO_EXP(h1 = test(data, 2048), "hash2 [2048]");
    TEST_NO_EXP(h2 = test(data, 1024), "hash2 [1024]");
    TEST(h1, h2, "hash2 [2048] == hash2 [1024]");

    // test small data with different buffer sizes
    data = rdata(8);
    TEST_NO_EXP(h1 = test(data, 7), "hash3 [7]");
    TEST_NO_EXP(h2 = test(data, 8), "hash3 [8]");
    TEST(h1, h2, "hash3 [7] == hash3 [8]");

    TEST_EXP(h1 = test(data, 3), "FORTE_MURMUR_TOO_SMALL", "hash3 [3] exception");
    TEST_NO_EXP(h1 = test(data, 4), "hash3 [4]");
    TEST(h1, h2, "hash3 [4] == hash3 [8]");

    // done
    return (all_pass ? 0 : 1);
}

