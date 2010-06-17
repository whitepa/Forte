#include "Forte.h"

using namespace Forte;

ServiceConfig::ServiceConfig() {
}
ServiceConfig::ServiceConfig(const char *configFile) {
    ReadConfigFile(configFile);
}

void ServiceConfig::ReadConfigFile(const char *configFile) {
    AutoUnlockMutex lock(mMutex);
    // load the file (INFO format)
    boost::property_tree::read_info(configFile, mPTree);
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

int ServiceConfig::GetInteger(const char *key)
{
    return mPTree.get<int>(key, 0);
}

