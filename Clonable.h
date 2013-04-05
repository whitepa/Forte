#ifndef __ClONEABLE_H__
#define __ClONEABLE_H__

namespace Forte {

class Clonable
{
public:
    virtual Clonable* Clone() const=0;
};

} // namespace Forte

#endif // __ClONEABLE_H__

