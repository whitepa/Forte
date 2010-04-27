#include "MockCurlFactory.h"
#include "MockCurl.h"

using namespace Forte;

CurlSharedPtr MockCurlFactory::CurlCreate()
{
    CurlSharedPtr c(new MockCurl());

    return c;
}
