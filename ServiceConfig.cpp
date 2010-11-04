#include "Foreach.h"
#include "FTrace.h"
#include "ServiceConfig.h"

using namespace Forte;

ServiceConfig::ServiceConfig() :
    mConfigFileType (ServiceConfig::UNKNOWN),
    mConfigFileName ("")
{
}
ServiceConfig::ServiceConfig(
    const char *configFile,
    ServiceConfig::ServiceConfigFileType type) :
    mConfigFileType (type),
    mConfigFileName (configFile)
{
    ReadConfigFile(configFile, type);
}

void ServiceConfig::ReadConfigFile(
    const char *configFile, 
    ServiceConfig::ServiceConfigFileType type) {
    AutoUnlockMutex lock(mMutex);
    mConfigFileType = type;
    mConfigFileName = configFile;
    // load the file (INFO format)
    try
    {
        switch (type)
        {
        case ServiceConfig::INI:
            boost::property_tree::read_ini(configFile, mPTree);
            break;
        case ServiceConfig::INFO:
            boost::property_tree::read_info(configFile, mPTree);
            break;
        default:
            throw EServiceConfig("invalid config file type specified");
        }
    }
    catch (boost::property_tree::ptree_error &e)
    {
        throw EServiceConfig("could not load file");
    }    

}

void ServiceConfig::Clear()
{
    AutoUnlockMutex lock(mMutex);
    mPTree = boost::property_tree::ptree();
}

void ServiceConfig::Set(const char *key, const char *value)
{
    AutoUnlockMutex lock(mMutex);
    try
    {
        mPTree.put(key, value);
    }
    catch (boost::property_tree::ptree_error &e)
    {
        throw EServiceConfig("unknown error");
    }
}

FString ServiceConfig::Get(const char *key)
{
    AutoUnlockMutex lock(mMutex);
    return mPTree.get<FString>(key, "");
}

void ServiceConfig::GetVectorSubKey(const char *key,     // nodes
                                    const char *subkey,  // backplane
                                    FStringVector &vec)
{
    FTRACE;
    vec.clear();
    boost::property_tree::ptree subtree = mPTree.get_child(key);
    foreach(const boost::property_tree::ptree::value_type &v, subtree)
    {
//        FString s1(v.first);
//        hlog(HLOG_DEBUG, "found tree %s", s1.c_str());
        // iterate keys in v.second
//        foreach(const boost::property_tree::ptree::value_type &v2, v.second)
//        {
//            FString s(v2.first);
//            hlog(HLOG_DEBUG, "   found subkey %s", s.c_str());
//        }
        vec.push_back(v.second.get<FString>(subkey));
    }
}

int ServiceConfig::GetInteger(const char *key)
{
    return mPTree.get<int>(key, 0);
}

void ServiceConfig::WriteToConfigFile(void)
{
    WriteToConfigFile(mConfigFileName.c_str());
}

void ServiceConfig::WriteToConfigFile(const char *newConfigFile)
{
    AutoUnlockMutex lock(mMutex);
    
    try
    {
        switch (mConfigFileType)
        {
        case ServiceConfig::INI:
            boost::property_tree::write_ini(newConfigFile, mPTree);
            break;
        case ServiceConfig::INFO:
            boost::property_tree::write_info(newConfigFile, mPTree);
            break;
        default:
            throw EServiceConfig("invalid config file type specified");
        }
    }
    catch (boost::property_tree::ptree_error &e)
    {
        throw EServiceConfig("could not write to config file");
    }
}
