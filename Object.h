#ifndef __forte__object_h_
#define __forte__object_h_
#include <boost/shared_ptr.hpp>
#include <map>
#include "FString.h"


namespace Forte
{
    class Object
    {
    public:
        virtual ~Object() {};
    };
    typedef boost::shared_ptr<Object> ObjectPtr;
    typedef std::map<FString, ObjectPtr> ObjectMap;
};

#endif
