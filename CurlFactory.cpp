// CurlFactory.cpp
#ifndef FORTE_NO_CURL

#include "CurlFactory.h"

using namespace Forte;
// statics
Mutex CurlFactory::sMutex;
CurlFactory* CurlFactory::sSingleton = NULL;

CurlFactory& CurlFactory::Get()
{
    // double-checked locking
    if (sSingleton == NULL)
    {
        AutoUnlockMutex lock(sMutex);
        if (sSingleton == NULL) sSingleton = new CurlFactory();
    }

    if (sSingleton == NULL)
    {
        throw EEmptyReference("CurlFactory pointer is invalid");
    }

    return *sSingleton;
}


// ctor/dtor
CurlFactory::CurlFactory()
{
}


CurlFactory::~CurlFactory()
{
}

CurlSharedPtr CurlFactory::CurlCreate()
{
    CurlSharedPtr c(new Curl());

    return c;
}

#endif  // FORTE_NO_CURL

