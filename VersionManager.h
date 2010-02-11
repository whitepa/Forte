#ifndef __VersionManager_h
#define __VersionManager_h

class CUtilVersion;
class CVersionManager {
public:
    CVersionManager() {};
    virtual ~CVersionManager() {};

    static inline void RegisterVersionString(const char *str)
    {
        CAutoUnlockMutex lock(sLock);
        sVersionStrings.push_back(std::string(str));
    }
    static inline void VersionVector(std::vector<std::string> &vec)
    {
        vec.clear();
        CAutoUnlockMutex lock(sLock);
        vec = sVersionStrings;
    }
protected:
    static CMutex sLock;
    static std::vector<std::string> sVersionStrings;
    static CUtilVersion sUtilVersion;
};

class CUtilVersion {
public:
    CUtilVersion();
    virtual ~CUtilVersion() {};
};

#endif
