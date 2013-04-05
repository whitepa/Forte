#ifndef __CRC32C_INTEL_SSE4_H__
#define __CRC32C_INTEL_SSE4_H__

#include "Clonable.h"
#include "CRC.h"

namespace Forte {

template <unsigned int initial>
class CRC32CIntelSSE4
    : public CRC, public Clonable
{
public:
    typedef CRC32CIntelSSE4 this_type;

    CRC32CIntelSSE4()
        :mChecksum(initial)
    {
    }

    void ProcessBytes(void const *buffer,
                      const std::size_t& length)
    {
        mChecksum = processBytes(mChecksum, buffer, length);
    }

    uint32_t GetChecksum() const
    {
        return mChecksum;
    }

    void Reset(const uint32_t& initialValue)
    {
        mChecksum = initialValue;
    }

    this_type* Clone() const
    {
        this_type* crc(new this_type());
        crc->mChecksum = mChecksum;
        return crc;
    }

    void operator()(const uint64_t& value)
    {
        mChecksum = __builtin_ia32_crc32di(mChecksum, value);
    }

    void operator()(const uint32_t& value)
    {
        mChecksum = __builtin_ia32_crc32si(mChecksum, value);
    }

    void operator()(const uint16_t& value)
    {
        mChecksum = __builtin_ia32_crc32hi(mChecksum, value);
    }

    void operator()(const uint8_t& value)
    {
        mChecksum = __builtin_ia32_crc32qi(mChecksum, value);
    }

protected:
    uint32_t processBytes(uint32_t crc,
                          void const *buffer,
                          std::size_t length)
    {
        const char* const pBufferEnd(reinterpret_cast<const char* const>(buffer) + length);
        const char* pBuffer(reinterpret_cast<const char*>(buffer));

        uint64_t crc64(crc);
        while(static_cast<std::size_t>(pBufferEnd - pBuffer) >= sizeof(uint64_t))
        {
            crc64 = __builtin_ia32_crc32di(crc64, *reinterpret_cast<const uint64_t*>(pBuffer));
            pBuffer += sizeof(uint64_t);
        }
        crc = static_cast<uint32_t>(crc64);

        if(static_cast<std::size_t>(pBufferEnd - pBuffer) >= sizeof(uint32_t))
        {
            crc = __builtin_ia32_crc32si(crc, *reinterpret_cast<const uint32_t*>(pBuffer));
            pBuffer += sizeof(uint32_t);
        }

        if(static_cast<std::size_t>(pBufferEnd - pBuffer) >= sizeof(uint16_t))
        {
            crc = __builtin_ia32_crc32hi(crc, *reinterpret_cast<const uint16_t*>(pBuffer));
            pBuffer += sizeof(uint16_t);
        }

        if(static_cast<std::size_t>(pBufferEnd - pBuffer) >= sizeof(uint8_t))
        {
            crc = __builtin_ia32_crc32qi(crc, *reinterpret_cast<const uint8_t*>(pBuffer));
        }

        return crc;
    }

private:
    uint32_t mChecksum;
};

} // namespace Forte

#endif // __CRC32C_INTEL_SSE4_H__

