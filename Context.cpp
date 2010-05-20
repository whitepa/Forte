#include "Context.h"
#include "FTrace.h"

Forte::Context::Context()
{
    FTRACE;
}

Forte::Context::Context(const Context &other)
{
    throw EUnimplemented(); // TODO remove me
    AutoUnlockMutex lock(other.mLock);
    mObjectMap = other.mObjectMap;
}

Forte::Context::~Context()
{
    FTRACE;
}

Forte::ObjectPtr Forte::Context::Get(const char *key) const
{
    ObjectMap::const_iterator i;
    Forte::AutoUnlockMutex lock(mLock);
    if ((i = mObjectMap.find(key)) == mObjectMap.end())
        // TODO: use a factory to create one?
        throw EInvalidKey();
    return (*i).second;
}

void Forte::Context::Set(const char *key, ObjectPtr obj) {
    Forte::AutoUnlockMutex lock(mLock);
    mObjectMap[key] = obj;
}

void Forte::Context::Remove(const char *key)
{
    // we must not cause object deletions while holding the lock
    ObjectPtr obj;
    {
        Forte::AutoUnlockMutex lock(mLock);
        if (mObjectMap.find(key) != mObjectMap.end())
        {
            obj = mObjectMap[key];
            mObjectMap.erase(key);
        }
    }
    // object goes out of scope and is deleted here, after the
    // lock has been released.
}

void Forte::Context::Clear(void)
{
    // we must not cause object deletions while holding the lock
    ObjectMap localCopy;
    {
        Forte::AutoUnlockMutex lock(mLock);
        localCopy = mObjectMap;
        mObjectMap.clear();
    }
}

