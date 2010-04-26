#include "MockCurlFactory.h"
#include "MockCurl.h"

using namespace Forte;

CurlFactory * MockCurlFactory::s_real_fs = NULL;
MockCurlFactory * MockCurlFactory::s_mock_fs = NULL;

void MockCurlFactory::SetupSingleton()
{
    Get(); // make sure we create a real singleton first
    CAutoUnlockMutex lock(CurlFactory::sMutex);
    if (s_real_fs == NULL) // must be a real FS in the singleton
        s_real_fs = CurlFactory::sSingleton;
    if (s_mock_fs == NULL)
        s_mock_fs = new MockCurlFactory();
    CurlFactory::sSingleton = s_mock_fs;
}

void MockCurlFactory::RealSingleton()
{
    Get(); // make sure we create a real singleton first
    CAutoUnlockMutex lock(CurlFactory::sMutex);
    if (s_real_fs == NULL) // must be a real FS in the singleton
        s_real_fs = CurlFactory::sSingleton;
    else
        CurlFactory::sSingleton = s_real_fs;
}


CurlSharedPtr MockCurlFactory::CurlCreate()
{
    CurlSharedPtr c(new MockCurl());

    return c;
}
