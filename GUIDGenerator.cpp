#include "Base64.h"
#include "GUIDGenerator.h"
#include "RandomGenerator.h"
#include <sys/time.h>
#include <sstream>

using namespace Forte;

Forte::GUIDGenerator::GUIDGenerator() :
    mRand(mRG.GetRandomUInt()), // seed the mt19937 generator from /dev/urandom
    mUUIDGen(&mRand)            // create a UUID generator with the mt19937 gen
{
}

FString & Forte::GUIDGenerator::GenerateGUID(FString &out, bool pathSafe)
{
    // generate a GUID
    boost::uuids::uuid u = mUUIDGen();
    std::stringstream ss;
    ss << u;
    out = ss.str();
    return out;
}
