#ifndef __forte_CurlFactory_h
#define __forte_CurlFactory_h

#include "Object.h"
#include <boost/shared_ptr.hpp>
#include "Curl.h"
#include "AutoMutex.h"


namespace Forte
{
    /**
     * CurlFactory is a class to create Curl objects. The singleton can be
     * replactioned out with a MockCurlFactory for use in unit tests
     **/

    class CurlFactory
    {
    public:
        // singleton
        static CurlFactory& Get();

        /**
         * Create a new curl object
         **/
        virtual CurlSharedPtr CurlCreate();

    protected:

        // ctor/dtor/singleton
        CurlFactory();
        virtual ~CurlFactory();
        static CurlFactory *sSingleton;
        static Mutex sMutex;
    };
    
}


#endif
