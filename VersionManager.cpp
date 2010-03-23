#include "Forte.h"

Mutex VersionManager::sLock;
std::vector<std::string> VersionManager::sVersionStrings;

UtilVersion VersionManager::sUtilVersion;


UtilVersion::UtilVersion()
{ 
    FString ver;
    ver.Format("forte library version 1.1.0 revision %s", REVISION);
    VersionManager::RegisterVersionString(ver);
}
