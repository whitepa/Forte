#ifndef __CRC32C_OPTIMALH__
#define __CRC32C_OPTIMALH__

#include "CRCOptimal.h"
#include <cstddef>

namespace Forte {

template <std::size_t initial=0>
class CRC32COptimal
    : public CRCOptimal<32, 0x1EDC6F41, initial, 0, true, true>
{
public:
    CRC32COptimal()
    {
    }
};

} // namespace Forte

#endif // __CRC32C_OPTIMALH__

