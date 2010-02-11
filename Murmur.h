#ifndef __forte_Murmur_h
#define __forte_Murmur_h

#include "Types.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(CForteException, CForteMurmerException);


    class Murmur64
    {
    public:
        Murmur64();
        void Init(uint64_t seed = 0xAAAAAAAAAAAAAAAALL);
        void Update(const void *data, size_t size);
        void Final();
        inline uint64_t GetHash() const { return m_h; }
        inline operator uint64_t() const { return m_h; }
        FString toStr() const;
        inline operator FString() const { return toStr(); }

    private:
        uint64_t m_k;
        uint64_t m_h;
        uint64_t m_len;
        static const uint64_t c_m;
        static const uint64_t c_r;
    };
};
#endif
