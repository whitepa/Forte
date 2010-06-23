#ifndef ServiceConfig_h
#define ServiceConfig_h

#include "Types.h"
#include "AutoMutex.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

namespace Forte
{
    EXCEPTION_CLASS(EServiceConfig);
    EXCEPTION_SUBCLASS2(EServiceConfig, EServiceConfigNoKey, "config key does not exist");

    /** 
     * \class ServiceConfig is a wrapper class around a boost property
     * tree, and is used to manage the configuration of a daemon (a
     * service).  Configuration files must be in INFO format, described at:
     * http://www.boost.org/doc/libs/1_43_0/doc/html/boost_propertytree/parsers.html#boost_propertytree.parsers.info_parser
     * 
     * 
     */
    class ServiceConfig : public Object {
    public:
        ServiceConfig();
        /** 
         * Create a ServiceConfig object, and parse the specified
         * configuration file.
         * 
         * @param configFile path to the configuration file.
         * 
         * @return 
         */
        ServiceConfig(const char *configFile);

        /** 
         * Read the configuration file at the given path, and populate
         * the internal property tree.  The resulting configuration
         * will the be union of the previous configuration, and the
         * information contained in the given configFile.
         * 
         * @param configFile 
         */
        void ReadConfigFile(const char *configFile);

        /** 
         * Set a given key to the given value.  Previous information
         * will be overwritten.
         *
         * @throws EServiceConfig on any error.
         * 
         * @param key 
         * @param value 
         */
        void Set(const char *key, const char *value);

        
        FString Get(const char *key);
        int GetInteger(const char *key);
        void Display(void);
        void Clear();

        /** 
         * GetVectorSubKey will retrieve a vector of values from the configuration.
         * 
         * @param key the config key which contains a vector of subtrees.
         * @param subkey the key within each subtree to retrieve.
         * @param vec the output vector of string config values.
         */
        void GetVectorSubKey(const char *key,
                             const char *subkey,
                             FStringVector &vec);

    protected:

        Mutex mMutex;
        boost::property_tree::ptree mPTree;
    };
};
#endif
