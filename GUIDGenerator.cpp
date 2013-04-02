#include "Base64.h"
#include "Foreach.h"
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

void Forte::GUIDGenerator::GenerateGUID(uint8_t out[])
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
    boost::uuids::uuid u;

    {
        AutoUnlockMutex lock(mLock);
        u = mUUIDGen();
    }

    out.reserve(u.size());
    std::copy(u.begin(), u.end(), std::back_inserter(out));
}

void Forte::GUIDGenerator::ToString(
    std::string& out,
    const std::vector<uint8_t>& existing)
{
    if (existing.size() != 16)
    {
        throw(EGUIDGenerator("got unexepected size for uuid"));
    }
    ToString(out, existing.data());
}

void Forte::GUIDGenerator::ToString(
    std::string& out,
    const uint8_t existing[])
{
    std::stringstream s;
    for (unsigned int i=0; i<16; i++)
    {
        if (i == 4
            || i == 6
            || i == 8
            || i == 10)
        {
            s << "-";
        }

        s << std::setw(2) << std::setfill('0') << std::hex << (int) existing[i];
    }
    out = s.str();
}
