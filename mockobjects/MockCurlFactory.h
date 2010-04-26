#ifndef __forte_MockCurlFactory_h
#define __forte_MockCurlFactory_h

#include "CurlFactory.h"

namespace Forte
{
    class MockCurlFactory : public CurlFactory
    {
    public:
        static void SetupSingleton();
        static void RealSingleton();
    public:

        CurlSharedPtr CurlCreate();

    protected:
        MockCurlFactory() {};
        virtual ~MockCurlFactory() {};
        static CurlFactory *s_real_fs;
        static MockCurlFactory *s_mock_fs;

    };
}

#endif
