// Murmur.cpp
#include "Murmur.h"
#include "LogManager.h"
#include "Types.h"
using namespace Forte;
// #define DEBUG_MURMUR

// modified Murmur64 hash
const uint64_t Murmur64::c_m = 0xc6a4a7935bd1e995;
const uint64_t Murmur64::c_r = 47;

Murmur64::Murmur64()
{
    Init();
}


void Murmur64::Init(uint64_t seed)
{
    mK = 0;
    mHash = seed;
    mLen = 0;
}


void Murmur64::Update(const void *data, size_t size)
{
    const char *cdata = reinterpret_cast<const char*>(data);
    const size_t b = (mLen % 8);
    const size_t n = (8 - b) % 8;

    // verify
    if (n > size) throw ForteMurmerException("FORTE_MURMUR_TOO_SMALL");

#ifdef DEBUG_MURMUR
    // debug logging
    if (n)
    {
        uint64_t x = 0;
        memcpy(&x, data, min<int>(size, 8));
        hlog(HLOG_DEBUG4, "mK = %016llx  [on entry]", mK);
        hlog(HLOG_DEBUG4, "data = %016llx", x);
    }
#endif

    // do n bytes to align to 8-byte boundary
    switch (n)
    {
    case 7:
    case 6:
    case 5:
    case 4:
    case 3:
    case 2:
    case 1:
    {
        char *ck = reinterpret_cast<char*>(&mK);
        memcpy(ck + b, cdata, n);
#ifdef DEBUG_MURMUR
        hlog(HLOG_DEBUG4, "mK = %016llx  [after alignment]", mK);
        hlog(HLOG_DEBUG4, "k = %016llx", mK);
#endif
        mK *= c_m;
        mK ^= mK >> c_r;
        mK *= c_m;
        mHash ^= mK;
        mHash *= c_m;
        mK = 0;
        break;
    }
    case 0:
    default:
        break;
    }

    // do 8-byte chunks
    const uint64_t *data64 = reinterpret_cast<const uint64_t*>(cdata + n);
    const uint64_t *end64 = data64 + ((size - n) / 8);
    const size_t leftover = ((size - n) % 8);
    uint64_t k;

    while (data64 != end64)
    {
        k = *data64++;
#ifdef DEBUG_MURMUR
        hlog(HLOG_DEBUG4, "k = %016llx", k);
#endif

        k *= c_m;
        k ^= k >> c_r;
        k *= c_m;

        mHash ^= k;
        mHash *= c_m;
    }

    // do leftover bytes
    const char *edata = reinterpret_cast<const char*>(end64);

    switch (leftover)
    {
    case 7:
    case 6:
    case 5:
    case 4:
    case 3:
    case 2:
    case 1:
        mK = 0;
        memcpy(&mK, edata, leftover);
#ifdef DEBUG_MURMUR
        hlog(HLOG_DEBUG4, "mK = %016llx  [leftover bytes]", mK);
#endif
        break;
    case 0:
    default:
        break;
    }

    // record amount completed
    mLen += size;
}


void Murmur64::Final()
{
    uint64_t final = mLen;

    // add length data
    Update(&final, 8);

    // apply any remaining bytes
    if (mK != 0)
    {
#ifdef DEBUG_MURMUR
        hlog(HLOG_DEBUG4, "k = %016llx", mK);
#endif
        mK *= c_m;
        mK ^= mK >> c_r;
        mK *= c_m;
        mHash ^= mK;
        mHash *= c_m;
        mK = 0;
    }

    // perform final mixing
    mHash ^= mHash >> c_r;
    mHash *= c_m;
    mHash ^= mHash >> c_r;
}


FString Murmur64::ToStr() const
{
    FString ret;
    ret.Format("%016llx", static_cast<unsigned long long>(mHash));
    return ret;
}

