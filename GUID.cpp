#include "Forte.h"
#include "GUID.h"

FString GUID::GenerateGUID(bool pathSafe)
{
    // generate a GUID
    // start with timestamp in microseconds
    struct timeval timeval;
    gettimeofday(&timeval, NULL);
    FString rawGUID;
    rawGUID.reserve(32);
    rawGUID.append((char *)&(timeval.tv_sec), 4);
    rawGUID.append((char *)&(timeval.tv_usec), 4);
    // get 16 bytes of secure random data
    rawGUID.append(Random::GetRandomData(16));
    // base64 encode
    FString GUID;
    CBase64::Encode(rawGUID, rawGUID.size(), GUID);
    if (pathSafe)
        GUID.Replace("/","_");
    return GUID; 
}
