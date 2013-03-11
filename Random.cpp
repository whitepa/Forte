#include "FString.h"
#include "Random.h"
#include "LogManager.h"
#include <fstream>

using namespace Forte;
using namespace std;

FString Random::GetSecureRandomData(unsigned int length)
{
    ifstream device;
    hlog(HLOG_WARN, "Random::GetSecureRandomData is deprecated");
    device.open("/dev/random", ios::in | ios::binary);
    if (!device.good())
        throw ERandom("unable to open /dev/random");
    char *data = new char[length+1];
    device.read(data, length);
    FString randomString(std::string(data, static_cast<size_t>(length)));
    delete [] data;
    return randomString;
}
FString Random::GetRandomData(unsigned int length)
{
    ifstream device;
    hlog(HLOG_WARN, "Random::GetSecureRandomData is deprecated");
    device.open("/dev/urandom", ios::in | ios::binary);
    if (!device.good())
        throw ERandom("unable to open /dev/urandom");
    char *data = new char[length+1];
    device.read(data, length);
    FString randomString(std::string(data, static_cast<size_t>(length)));
    delete [] data;
    return randomString;
}
unsigned int Random::GetRandomUInt(void)
{
    ifstream device;
    hlog(HLOG_WARN, "Random::GetSecureRandomData is deprecated");
    device.open("/dev/urandom", ios::in | ios::binary);
    if (!device.good())
        throw ERandom("unable to open /dev/urandom");
    unsigned int r;
    device.read(reinterpret_cast<char *>(&r), sizeof(unsigned int));
    return r;
}
