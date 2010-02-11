#include "Forte.h"

CMutex CVersionManager::sLock;
std::vector<std::string> CVersionManager::sVersionStrings;

CUtilVersion CVersionManager::sUtilVersion;


CUtilVersion::CUtilVersion()
{ 
    FString ver;
    ver.Format("forte library version 1.1.0 revision %s", REVISION);
    CVersionManager::RegisterVersionString(ver);
}
