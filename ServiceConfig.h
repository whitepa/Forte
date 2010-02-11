#ifndef ServiceConfig_h
#define ServiceConfig_h

#include "Types.h"
#include "AutoMutex.h"

class CServiceConfig {
public:
    CServiceConfig();
    CServiceConfig(const char *configFile);

    void ReadConfigFile(const char *configFile);

    void Set(const char *key, const char *value);
    FString Get(const char *key);
    int GetInteger(const char *key);
    void Display(void);
    void Clear();

protected:

    CMutex mMutex;
    StringHashMap mConfig;
};

#endif
