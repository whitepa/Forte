#ifndef __forte_Murmur_h
#define __forte_Murmur_h

#include "Types.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, ForteMurmerException);


    class Murmur64
    {
    public:
        Murmur64();
        void Init(uint64_t seed = 0xAAAAAAAAAAAAAAAALL);
        void Update(const void *data, size_t size);
        void Final();
        inline uint64_t GetHash() const { return mHash; }
        inline operator uint64_t() const { return mHash; }
        FString ToStr() const;
        inline operator FString() const { return ToStr(); }

    private:
        uint64_t mK;
        uint64_t mHash;
        uint64_t mLen;
        static const uint64_t c_m;
        static const uint64_t c_r;
    };
};
#endif
