#include "FString.h"
#include "RandomGenerator.h"
#include "LogManager.h"
#include "SystemCallUtil.h"
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace Forte;
using namespace std;

Forte::RandomGenerator::RandomGenerator()
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
        throw ERandomGeneratorDevice();
    mRandomDevice = fd;
}
FString & Forte::RandomGenerator::GetRandomData(unsigned int length, FString &out)
{
    out.resize(length, 0);
    int ret = 0;
    do
    {
        ret = read(mRandomDevice, const_cast<char *>(out.c_str()), length);
    }
    while (ret != static_cast<int>(length) && errno == EINTR);
    if (ret != static_cast<int>(length))
        throw ERandomGeneratorFailed(SystemCallUtil::GetErrorDescription(errno));
    return out;
}
unsigned int Forte::RandomGenerator::GetRandomUInt(void)
{
    unsigned int r;
    FString data;
    GetRandomData(sizeof(unsigned int), data);
    memcpy(reinterpret_cast<void *>(&r),
           static_cast<const void *>(data.c_str()),
           sizeof(unsigned int));
    return r;
}
