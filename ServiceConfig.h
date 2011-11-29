#ifndef ServiceConfig_h
#define ServiceConfig_h

#include "Types.h"
#include "AutoMutex.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>

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

        enum ServiceConfigFileType
        {
            INI,
            INFO,
            UNKNOWN
        };

        ServiceConfig();
        /**
         * Create a ServiceConfig object, and parse the specified
         * configuration file.
         *
         * @param configFile path to the configuration file.
         *
         * @return
         */
        ServiceConfig(const char *configFile, ServiceConfigFileType type = INFO);

        /**
         * Read the configuration file at the given path, and populate
         * the internal property tree.  The resulting configuration
         * will the be union of the previous configuration, and the
         * information contained in the given configFile.
         *
         * @param configFile
         */
        void ReadConfigFile(const char *configFile, ServiceConfigFileType type = INFO);

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

        /**
         * Set a given key to the given value.  If previous
         * information exists at the key, a sibling key is created.
         *
         * @throws EServiceConfig on any error.
         *
         * @param key
         * @param value
         */
        void Add(const char *key, const char *value);

        /* 
         * Removes a given key.
         *
         * @throws EServiceConfig on any error.
         *
         * @param key
         */
        void Erase(const char *key);

        FString Get(const char *key);
        int GetInteger(const char *key);

        /**
         * Get() this is templated, and also throws an exception
         * if the key does not exist
         *
         * @params key the key to find which is of type ptree::path_type
         * @return the value of the key
         * @throws EServiceConfigNoKey if key doesnot exist or error
         *         converting the value to ValueType
         */
        template <typename ValueType>
        ValueType Get(const boost::property_tree::ptree::path_type &key)
        {
            AutoUnlockMutex lock(mMutex);
            try
            {
                return mPTree.get<ValueType>(key);
            }
            catch (boost::property_tree::ptree_error &e)
            {
                boost::throw_exception(EServiceConfigNoKey());
            }
        }

        void Display(void);
        void Clear();

        /**
         * GetVectorKeys will retrieve a vector of subkeys for a given key
         *
         * @param key the config key which contains subtrees
         * @param vec the output vector of the subkeys under key
         */
        void GetVectorKeys(
            const char *key,
            FStringVector &vec /*OUT*/);
        void GetVectorKeys(
            const boost::property_tree::ptree::path_type &key,
            FStringVector &vec /*OUT*/);

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

        /**
         * Write the configuration back to the service config
         * file that it read from
         */
        void WriteToConfigFile(void);

        /**
         * Write the configuration to a new config file
         *
         * @param newConfigFile the new config file to write.
         */
        void WriteToConfigFile(const char *newConfigFile);

        /**
         * Gets the first property containing a regex expression that
         * matches the child key given. For example the child key "a.1.B"
         * would match the regular expression is "a\..*\.B"
         *
         * @param parentKey key containing subkeys that are regular expressions
         * @param childToMatch key to actually match
         */
        FString GetFirstMatchingRegexExpressionKey(const char *parentKey,
                const char* childToMatch);


        FString GetConfigFileName() {
            return mConfigFileName;
        }

    protected:

        ServiceConfigFileType mConfigFileType;
        std::string mConfigFileName;
        Mutex mMutex;
        boost::property_tree::ptree mPTree;
    };
    typedef boost::shared_ptr<ServiceConfig> ServiceConfigPtr;
};
#endif
