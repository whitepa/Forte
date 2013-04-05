#ifndef __CRCBOOST_H__
#define __CRCBOOST_H__

#include "Clonable.h"
#include "CRC.h"
#include <boost/crc.hpp>
#include <cstddef>

namespace Forte {

template <std::size_t bits, unsigned int polynomial,
          unsigned int initial, unsigned int finalXOR,
          bool reverseData, bool reverseResult>
class CRCBoost
    : public CRC, public Clonable
{
public:
    typedef CRCBoost this_type;

    CRCBoost()
    {
    }

    CRCBoost(const CRCBoost& o)
        :mAgent(o.mAgent)
    {
    }

    void ProcessBytes(void const *buffer,
                      const std::size_t& length)
    {
        mAgent.process_bytes(buffer, length);
    }

    uint32_t GetChecksum() const
    {
        return mAgent.checksum();
    }

    void Reset(const uint32_t& initialValue)
    {
        mAgent.reset(initialValue);
    }

    this_type* Clone() const
    {
        this_type* crc(new this_type());
        crc->mAgent = mAgent;
        return crc;
    }

    this_type& operator=(const this_type& rhs)
    {
        if(this != &rhs)
        {
            mAgent = rhs.mAgent;
        }
        return *this;
    }

private:
    typedef boost::crc_optimal<bits, polynomial,
                               initial, finalXOR,
                               reverseData, reverseResult>
            crc_32_type;

    crc_32_type mAgent;
};

} // namespace Forte

#endif // __CRCBOOST_H__

