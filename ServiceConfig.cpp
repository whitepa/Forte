#include "Foreach.h"
#include "FTrace.h"
#include "ServiceConfig.h"

using namespace Forte;

ServiceConfig::ServiceConfig() :
    mConfigFileType (ServiceConfig::UNKNOWN),
    mConfigFileName ("")
{
}
ServiceConfig::ServiceConfig(const char *configFile,
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
        FString stmp;
        stmp.Format("could not load file %s", mConfigFileName.c_str());
        throw EServiceConfig(stmp);
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
void ServiceConfig::Add(const char *key, const char *value)
{
    AutoUnlockMutex lock(mMutex);
    try
    {
        mPTree.add(key, value);
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
    try
    {
        foreach(const boost::property_tree::ptree::value_type &v, mPTree.get_child(key))
        {
            FString s1(v.first);
            hlog(HLOG_DEBUG, "found %s", s1.c_str());
            vec.push_back(v.second.get<FString>(subkey));
        }
    }
    catch (boost::property_tree::ptree_error &e)
    {
        boost::throw_exception(EServiceConfigNoKey(key));
    }
}

int ServiceConfig::GetInteger(const char *key)
{
    return mPTree.get<int>(key, 0);
}

boost::property_tree::ptree& ServiceConfig::GetChildTree(
    const char *key)
{
    AutoUnlockMutex lock(mMutex);
    try
    {
        return mPTree.get_child(key);
    }    
    catch (boost::property_tree::ptree_error &e)
    {
        boost::throw_exception(EServiceConfigNoKey(key));
    }
}

void ServiceConfig::GetVectorKeys(
    const char *key,
    FStringVector &vec /*OUT*/)
{
    vec.clear();
    boost::property_tree::ptree& childTree = GetChildTree(key);

    foreach (const boost::property_tree::ptree::value_type &v, childTree)
    {
        vec.push_back(v.first);
    }
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
        FString stmp;
        stmp.Format("could not write to config file : %s", newConfigFile);
        throw EServiceConfig(stmp);
    }
}
