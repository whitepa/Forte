#ifndef ServiceConfig_h
#define ServiceConfig_h

#include "Types.h"
#include "AutoMutex.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

namespace Forte
{
    class ServiceConfig : public Object {
    public:
        ServiceConfig();
        ServiceConfig(const char *configFile);

        void ReadConfigFile(const char *configFile);

        void Set(const char *key, const char *value);
        FString Get(const char *key);
        int GetInteger(const char *key);
        void Display(void);
        void Clear();

    protected:

        Mutex mMutex;
        boost::property_tree::ptree mPTree;
    };
};
#endif
