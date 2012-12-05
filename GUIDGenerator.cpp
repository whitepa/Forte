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

FString & Forte::GUIDGenerator::GenerateGUID(FString &out)
{
    // generate a GUID
    AutoUnlockMutex lock(mLock);
    boost::uuids::uuid u = mUUIDGen();
    std::stringstream ss;
    ss << u;
    out = ss.str();
    return out;
}

void Forte::GUIDGenerator::GenerateGUID(uint8_t &out[])
{
    std::vector<uint8_t> v;
    GenerateGUID(v);
    int i=0;
    foreach(const uint8_t &u,v)
        out[i++]=u;
}

void Forte::GUIDGenerator::GenerateGUID(std::vector<uint8_t> &out)
{
    out.clear();
    AutoUnlockMutex lock(mLock);
    boost::uuids::uuid u = mUUIDGen();
    out.reserve(u.size());
    std::copy(u.begin(), u.end(), v.begin());
}
