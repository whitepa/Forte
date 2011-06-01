#ifndef __forte_property_object_h_
#define __forte_property_object_h_

#include "AutoMutex.h"
#include "FString.h"
#include "Exception.h"
#include "Object.h"
#include "Types.h"

namespace Forte
{
    EXCEPTION_CLASS(EPropertyObject);
    EXCEPTION_SUBCLASS2(EPropertyObject, EPropertyObjectNoKey, "Key does not exist");
    
    class PropertyObject : public Object
    {
    public:
        PropertyObject() {}
        virtual ~PropertyObject() {}

        PropertyObject(const PropertyObject &other) {
            *this = other;
        };
        const PropertyObject & operator= (const PropertyObject &rhs) {
            AutoUnlockMutex lock(rhs.mPropertyLock);
            mProperties = rhs.mProperties;
            return *this;
        };
        void SetProperty(const FString &key, const FString &value) {
            AutoUnlockMutex lock(mPropertyLock);
            mProperties[key] = value;
        };
        FString GetProperty(const FString &key) {
            AutoUnlockMutex lock(mPropertyLock);
            StrStrMap::iterator i = mProperties.find(key);
            if (i == mProperties.end())
                throw EPropertyObjectNoKey(key);
            return i->second;
        };
    private:
        mutable Mutex mPropertyLock;
        StrStrMap mProperties;
    };
}

#endif
