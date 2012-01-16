#include "Foreach.h"
#include "FTrace.h"
#include "ServiceConfig.h"
#include <boost/regex.hpp>

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

void ServiceConfig::Erase(const char *key)
{
    AutoUnlockMutex lock(mMutex);
    try
    {
        mPTree.erase(key);
    }
    catch (boost::property_tree::ptree_error &e)
    {
        throw EServiceConfig(e.what());
    }
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
    vec.clear();
    AutoUnlockMutex lock(mMutex);
    try
    {
        foreach(const boost::property_tree::ptree::value_type &v, mPTree.get_child(key))
        {
            FString s1(v.first);
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
    AutoUnlockMutex lock(mMutex);
    try
    {
        return mPTree.get<int>(key);
    }
    catch (boost::property_tree::ptree_error &e)
    {
        hlog(HLOG_ERR, "error getting key %s : %s",
             key, e.what());
        boost::throw_exception(EServiceConfigNoKey(key));
    }
}

int ServiceConfig::getInt(const char *key)
{
    int value;

    FString rootChild(key);
    rootChild.LeftString(".");

    FString leafName(key);
    leafName.RightString(".");

    FString middleStuff(key);
    middleStuff.ChopRight(".");
    middleStuff.ChopLeft(".");

    FString propertyKey;

    try
    {
        propertyKey = GetFirstMatchingRegexExpressionKey(rootChild, middleStuff);
    }
    catch (EServiceConfigNoKey &e)
    {
        hlog(HLOG_DEBUG, "Can't find regex match for (%s) under rootChild (%s)", middleStuff.c_str(), rootChild.c_str());
        throw;
    }

    propertyKey.append(FString("/"));
    propertyKey.append(leafName);

    try
    {
        value = Get<int>(boost::property_tree::ptree::path_type(propertyKey.c_str(), '/'));
        return value;
    }
    catch (boost::property_tree::ptree_bad_path &e)
    {
        hlog(HLOG_DEBUG, "No value for key: %s", key);
        boost::throw_exception(EServiceConfigNoKey(key));
    }
    catch (boost::property_tree::ptree_error &e)
    {
        hlog(HLOG_ERR, "Error in query, conf file, or data: key(%s), error(%s)",
             key, e.what());
        boost::throw_exception(EServiceConfigNoKey(key));
    }
}

FString ServiceConfig::getString(const char *key)
{
    FString value;

    FString rootChild(key);
    rootChild.LeftString(".");

    FString leafName(key);
    leafName.RightString(".");

    FString middleStuff(key);
    middleStuff.ChopRight(".");
    middleStuff.ChopLeft(".");

    FString propertyKey;

    try
    {
        propertyKey = GetFirstMatchingRegexExpressionKey(rootChild, middleStuff);
    }
    catch (EServiceConfigNoKey &e)
    {
        hlog(HLOG_DEBUG, "Can't find regex match for (%s) under rootChild (%s)", middleStuff.c_str(), rootChild.c_str());
        throw;
    }

    propertyKey.append(FString("/"));
    propertyKey.append(leafName);

    try
    {
        value = Get<std::string>(boost::property_tree::ptree::path_type(propertyKey.c_str(), '/'));
        return value;
    }
    catch (boost::property_tree::ptree_bad_path &e)
    {
        hlog(HLOG_DEBUG, "No value for key: %s", key);
        boost::throw_exception(EServiceConfigNoKey(key));
    }
    catch (boost::property_tree::ptree_error &e)
    {
        hlog(HLOG_ERR, "Error in query, conf file, or data: key(%s), error(%s)",
             key, e.what());
        boost::throw_exception(EServiceConfigNoKey(key));
    }
}

FString ServiceConfig::resolveString(const Forte::FString &key)
{
    FString value;
    FString default_key_string(key);

    FString leafName(key);
    leafName.RightString(".");

    try
    {
        value = ServiceConfig::getString(key);
        return value;
    }
    catch (EServiceConfigNoKey &e)
    {
        default_key_string.LeftString(".").append(".default.").append(leafName);
        try
        {
            value = ServiceConfig::getString(default_key_string);
            hlog(HLOG_DEBUG, "Found default value (%s) at location: (%s)", value.c_str(), default_key_string.c_str());
            return value;
        }
        catch (EServiceConfigNoKey &e)
        {
            // if this doesn't succeed, we have no legitimate value
            hlog(HLOG_WARN, "Serious error, conf variable resolver cannot return a legitimate value for key (%s)", default_key_string.c_str());
            throw;
        }
    }
}

int ServiceConfig::resolveInt(const Forte::FString &key)
{
    int value;
    FString default_key_string(key);

    FString leafName(key);
    leafName.RightString(".");

    try
    {
        value = ServiceConfig::getInt(key);
        return value;
    }
    catch (EServiceConfigNoKey &e)
    {
        default_key_string.LeftString(".").append(".default.").append(leafName);
        try
        {
            value = ServiceConfig::getInt(default_key_string);
            hlog(HLOG_DEBUG, "Found default value (%i) at location: (%s)", value, default_key_string.c_str());
            return value;
        }
        catch (EServiceConfigNoKey &e)
        {
            // if this doesn't succeed, we have no legitimate value
            hlog(HLOG_WARN, "Serious error, conf variable resolver cannot return a legitimate value for key (%s)", default_key_string.c_str());
            throw;
        }
    }
}

void ServiceConfig::GetVectorKeys(
    const char *key,
    FStringVector &vec /*OUT*/)
{
    vec.clear();

    AutoUnlockMutex lock(mMutex);
    try
    {
        foreach (const boost::property_tree::ptree::value_type &v, mPTree.get_child(key))
        {
            vec.push_back(v.first);
        }
    }
    catch (boost::property_tree::ptree_error &e)
    {
        boost::throw_exception(EServiceConfigNoKey(e.what()));
    }
}

void ServiceConfig::GetVectorKeys(
    const boost::property_tree::ptree::path_type &key,
    FStringVector &vec /*OUT*/)
{
    vec.clear();

    AutoUnlockMutex lock(mMutex);
    try
    {
        foreach (const boost::property_tree::ptree::value_type &v, mPTree.get_child(key))
        {
            vec.push_back(v.first);
        }
    }
    catch (boost::property_tree::ptree_error &e)
    {
        boost::throw_exception(EServiceConfigNoKey(e.what()));
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
        boost::throw_exception(EServiceConfig(stmp));
    }
}

Forte::FString ServiceConfig::GetFirstMatchingRegexExpressionKey(
        const char* parentKey,
        const char* childToMatch)
{
    Forte::FStringVector subKeyVector;
    GetVectorKeys(parentKey, subKeyVector);

    foreach (const FString &key, subKeyVector)
    {
        boost::regex e(key);
        if (regex_match(childToMatch, e))
        {
            return FString(FStringFC(), "%s/%s", parentKey, key.c_str());
        }
    }
    boost::throw_exception(EServiceConfigNoKey());
}
