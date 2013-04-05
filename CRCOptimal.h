#ifndef __CRC_OPTIMAL_H__
#define __CRC_OPTIMAL_H__

#include "CRCBoost.h"
#include "CRC32CIntelSSE4.h"
#include <boost/scoped_ptr.hpp>
#include <cstddef>
#include <cpuid.h>

namespace Forte {

template <std::size_t bits, unsigned int polynomial,
          unsigned int initial, unsigned int finalXOR,
          bool reverseData, bool reverseResult>
class CRCOptimal
    : public CRCBoost<bits, polynomial, initial, finalXOR, reverseData, reverseResult>
{
public:
    static const std::size_t BITS = bits;
    static const std::size_t POLYNOMIAL = polynomial;
    static const std::size_t INITIAL = initial;
    static const std::size_t FINAL_XOR = finalXOR;
    static const std::size_t REVERSE_DATA = reverseData;
    static const std::size_t REVERSE_RESULT = reverseResult;

    CRCOptimal()
    {
    }
};

template <unsigned int initial>
class CRCOptimal<32, 0x1EDC6F41, initial, 0, true, true>
    : public CRC
{
public:
    typedef CRCOptimal<32, 0x1EDC6F41, initial, 0, true, true> this_type;
    typedef CRC base_type;

    static const unsigned long BITS = 32;
    static const std::size_t POLYNOMIAL = 0x1EDC6F41;
    static const std::size_t INITIAL = initial;
    static const std::size_t FINAL_XOR = 0;
    static const std::size_t REVERSE_DATA = true;
    static const std::size_t REVERSE_RESULT = true;

    CRCOptimal()
        :mCRC(dynamic_cast<CRC*>(
            dynamic_cast<const Clonable*>(sCRC.mCRC.get())->Clone()))
    {
        mCRC->Reset(initial);
    }

    CRCOptimal(const this_type& o)
        :mCRC(dynamic_cast<CRC*>(
            dynamic_cast<const Clonable*>(o.mCRC.get())->Clone()))
    {
    }

    this_type& operator=(const this_type& rhs)
    {
        if(this != &rhs)
        {
            mCRC.reset(new this_type(rhs));
        }
        return *this;
    }

    void ProcessBytes(void const *buffer,
                      const std::size_t& length)
    {
        mCRC->ProcessBytes(buffer, length);
    }

    uint32_t GetChecksum() const
    {
        return mCRC->GetChecksum();
    }

    void Reset(const uint32_t& initialValue)
    {
        mCRC->Reset(initialValue);
    }

protected:
    template <unsigned int initialValue>
    struct CRCOptimalPrototype
    {
        typedef CRCBoost<32, 0x1EDC6F41, initialValue, 0, true, true> crc_generic_type;
        typedef CRC32CIntelSSE4<initialValue> crc32c_optimized_type;

        CRCOptimalPrototype()
            :mCRC(hasHardwareCRC()?
                  static_cast<base_type const*>(new crc32c_optimized_type()) :
                  static_cast<base_type const*>(new crc_generic_type()))
        {
        }

        bool hasHardwareCRC() const
        {
            unsigned int eax, ebx, ecx, edx;
            __get_cpuid(1, &eax, &ebx, &ecx, &edx);
            return (ecx & bit_SSE4_2);
        }

        boost::scoped_ptr<base_type const> mCRC;
    };

private:
    const boost::scoped_ptr<base_type> mCRC;
    static const CRCOptimalPrototype<initial> sCRC;
};

template <unsigned int initial>
CRCOptimal<32, 0x1EDC6F41, initial, 0, true, true>::CRCOptimalPrototype<initial>
const CRCOptimal<32, 0x1EDC6F41, initial, 0, true, true>::sCRC;

} // namespace Forte

#endif // __CRC_OPTIMAL_H__

