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
    m_k = 0;
    m_h = seed;
    m_len = 0;
}


void Murmur64::Update(const void *data, size_t size)
{
    const char *cdata = (const char*)(data);
    const size_t b = (m_len % 8);
    const size_t n = (8 - b) % 8;

    // verify
    if (n > size) throw CForteMurmerException("FORTE_MURMUR_TOO_SMALL");

#ifdef DEBUG_MURMUR
    // debug logging
    if (n)
    {
        uint64_t x = 0;
        memcpy(&x, data, min<int>(size, 8));
        hlog(HLOG_DEBUG4, "m_k = %016llx  [on entry]", m_k);
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
        char *ck = (char*)&m_k;
        memcpy(ck + b, cdata, n);
#ifdef DEBUG_MURMUR
        hlog(HLOG_DEBUG4, "m_k = %016llx  [after alignment]", m_k);
        hlog(HLOG_DEBUG4, "k = %016llx", m_k);
#endif
        m_k *= c_m;
        m_k ^= m_k >> c_r;
        m_k *= c_m;
        m_h ^= m_k;
        m_h *= c_m;
        m_k = 0;
        break;
    }
    case 0:
    default:
        break;
    }

    // do 8-byte chunks
    const uint64_t *data64 = (const uint64_t*)(cdata + n);
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

        m_h ^= k;
        m_h *= c_m;
    }

    // do leftover bytes
    const char *edata = (const char*)(end64);

    switch (leftover)
    {
    case 7:
    case 6:
    case 5:
    case 4:
    case 3:
    case 2:
    case 1:
        m_k = 0;
        memcpy(&m_k, edata, leftover);
#ifdef DEBUG_MURMUR
        hlog(HLOG_DEBUG4, "m_k = %016llx  [leftover bytes]", m_k);
#endif
        break;
    case 0:
    default:
        break;
    }

    // record amount completed
    m_len += size;
}


void Murmur64::Final()
{
    uint64_t final = m_len;

    // add length data
    Update(&final, 8);

    // apply any remaining bytes
    if (m_k != 0)
    {
#ifdef DEBUG_MURMUR
        hlog(HLOG_DEBUG4, "k = %016llx", m_k);
#endif
        m_k *= c_m;
        m_k ^= m_k >> c_r;
        m_k *= c_m;
        m_h ^= m_k;
        m_h *= c_m;
        m_k = 0;
    }

    // perform final mixing
    m_h ^= m_h >> c_r;
    m_h *= c_m;
    m_h ^= m_h >> c_r;
}


FString Murmur64::toStr() const
{
    FString ret;
    ret.Format("%016llx", (u64)m_h);
    return ret;
}

