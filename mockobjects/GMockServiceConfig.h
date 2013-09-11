#ifndef __GMockServiceConfig_h
#define __GMockServiceConfig_h

#include <gmock/gmock.h>
#include "ServiceConfig.h"

namespace Forte
{
    class GMockServiceConfig : public ServiceConfig
    {
    public:
        MOCK_METHOD2(ReadConfigFile, void (const char *, ServiceConfigFileType));
        MOCK_METHOD2(Set, void (const char*, const char *));
        MOCK_METHOD2(Add, void (const char*, const char*));
        MOCK_METHOD1(Erase, void(const char*));
        MOCK_METHOD2(CopyTree, void (const FString&, const FString&));
        MOCK_CONST_METHOD1(Get, FString(const char*));
        MOCK_CONST_METHOD1(GetInteger, int(const char*));
        MOCK_METHOD0(Clear, void(void));
        MOCK_CONST_METHOD2(GetVectorKeys, void(const char *, FStringVector&));
        MOCK_CONST_METHOD2(GetVectorKeys,
                           void(const boost::property_tree::ptree::path_type &,
                                FStringVector &));
        MOCK_CONST_METHOD3(GetVectorSubKey, void(const char*, const char*,
                                                 FStringVector&));
        MOCK_METHOD1(WriteToConfigFile, void(const char *));
        MOCK_METHOD2(GetFirstMatchingRegexExpressionKey,
                     FString(const char *, const char *));
        MOCK_CONST_METHOD0(GetConfigFileName, FString(void));

    protected:
        MOCK_METHOD1(getInt, int(const char*));
        MOCK_METHOD1(resolveInt, int(const Forte::FString&));
        MOCK_METHOD1(getString, FString(const char *));
        MOCK_METHOD1(resolveString, FString(const Forte::FString&));
    };
};

#endif
