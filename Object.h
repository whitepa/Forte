#ifndef __forte__object_h_
#define __forte__object_h_
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <map>
#include "FString.h"

namespace Forte
{
    class Object : public boost::enable_shared_from_this<Forte::Object>, 
                   private boost::noncopyable_::noncopyable
    {
    public:
        virtual ~Object() {};
    };
    typedef boost::shared_ptr<Object> ObjectPtr;
    typedef std::pair<FString, ObjectPtr> ObjectPair;
    typedef std::map<FString, ObjectPtr> ObjectMap;
};

#endif
