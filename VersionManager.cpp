#include "VersionManager.h"

using namespace Forte;

Mutex VersionManager::sLock;
std::vector<std::string> VersionManager::sVersionStrings;

UtilVersion VersionManager::sUtilVersion;


UtilVersion::UtilVersion()
{
    FString ver;
    ver.Format("forte library version 2.0.0");
    VersionManager::RegisterVersionString(ver);
}
