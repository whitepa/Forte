#ifndef __VersionManager_h
#define __VersionManager_h
namespace Forte
{
    class UtilVersion;
    class VersionManager {
    public:
        VersionManager() {};
        virtual ~VersionManager() {};

        static inline void RegisterVersionString(const char *str)
            {
                AutoUnlockMutex lock(sLock);
                sVersionStrings.push_back(std::string(str));
            }
        static inline void VersionVector(std::vector<std::string> &vec)
            {
                vec.clear();
                AutoUnlockMutex lock(sLock);
                vec = sVersionStrings;
            }
    protected:
        static Mutex sLock;
        static std::vector<std::string> sVersionStrings;
        static UtilVersion sUtilVersion;
    };

    class UtilVersion {
    public:
        UtilVersion();
        virtual ~UtilVersion() {};
    };
};
#endif
