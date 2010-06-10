#include "Forte.h"
#include "Random.h"

using namespace Forte;

FString Random::GetSecureRandomData(unsigned int length)
{
    ifstream device;
    device.open("/dev/random", ios::in | ios::binary);
    if (!device.good())
        throw ForteRandomException("unable to open /dev/random");
    char *data = new char[length+1];
    device.read(data, length);
    FString randomString(std::string(data, (size_t)length));
    delete [] data;
    return randomString;
}
FString Random::GetRandomData(unsigned int length)
{
    ifstream device;
    device.open("/dev/urandom", ios::in | ios::binary);
    if (!device.good())
        throw ForteRandomException("unable to open /dev/urandom");
    char *data = new char[length+1];
    device.read(data, length);
    FString randomString(std::string(data, (size_t)length));
    delete [] data;
    return randomString;
}
unsigned int Random::GetRandomUInt(void)
{
    ifstream device;
    device.open("/dev/urandom", ios::in | ios::binary);
    if (!device.good())
        throw ForteRandomException("unable to open /dev/urandom");
    unsigned int r;
    device.read((char *)&r, sizeof(unsigned int));
    return r;
}
