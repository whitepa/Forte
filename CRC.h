#ifndef __CRC_H__
#define __CRC_H__

#include <stdint.h>
#include <cstddef>
#include <iostream>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>

namespace Forte {

   /**
    * @class CRC
    * @brief This abstract class provides the interface to compute crc
    */
class CRC
{
public:
    virtual ~CRC()
    {
    }

    /**
     * Reads length bytes from buffer to add to accumulated crc
     * @param buffer input buffer to read from
     * @param length length of input buffer
     */
    virtual void ProcessBytes(void const *buffer, const std::size_t& length)=0;

    /**
     * Get the accumulated checksum
     * @return the checksum value
     */
    virtual uint32_t GetChecksum() const=0;

    /**
     * Reset to clear previous accumulation
     * @initial starting checksum value
     */
    virtual void Reset(const uint32_t& initial)=0;

    bool operator!=(const CRC& rhs) const
    {
        return (GetChecksum() != rhs.GetChecksum());
    }

    bool operator==(const CRC& rhs) const
    {
        return !(*this != rhs);
    }

    virtual void operator()(const uint64_t& value)
    {
        process(value);
    }

    virtual void operator()(const uint32_t& value)
    {
        process(value);
    }

    virtual void operator()(const uint16_t& value)
    {
        process(value);
    }

    virtual void operator()(const uint8_t& value)
    {
        process(value);
    }

protected:
    template <class T>
    typename boost::enable_if<boost::is_integral<T>, void>::type
    process(const T& value)
    {
        ProcessBytes(reinterpret_cast<void const*>(&value), sizeof(value));
    }

}; // CRC

inline std::ostream& operator<<(std::ostream& out, const CRC& obj)
{
    out << obj.GetChecksum();
    return out;
}

} // namespace Forte

#endif // __CRC_H__

