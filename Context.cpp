#include "Context.h"
#include "FTrace.h"
#include "Foreach.h"

Forte::Context::Context()
{
    FTRACE;
}

Forte::Context::~Context()
{
    FTRACE;
    if (!mObjectMap.empty())
    {
        hlog(HLOG_DEBUG, "Objects Remain in Context at deletion:");
        Dump();
    }
    ObjectMap tmp;
    {
        Forte::AutoUnlockMutex lock(mLock);
        tmp = mObjectMap;
        mObjectMap.clear();
    }
}

Forte::ObjectPtr Forte::Context::Get(const char *key) const
{
    ObjectMap::const_iterator i;
    Forte::AutoUnlockMutex lock(mLock);
    if ((i = mObjectMap.find(key)) == mObjectMap.end())
        // use a factory to create one?  NO.  Objects must be
        // explicitly created.  This avoids potential problems if
        // destructors decide to try to access context objects after
        // they have been removed.
        throw EInvalidKey();
    return (*i).second;
}

void Forte::Context::Set(const char *key, ObjectPtr obj)
{
    if (!obj)
    {
        // disallow setting to an empty pointer
        throw EEmptyPointer();
    }
    ObjectPtr replaced;
    {
        Forte::AutoUnlockMutex lock(mLock);
        if (mObjectMap.find(key) != mObjectMap.end())
        {
            replaced = mObjectMap[key];
        }        
        mObjectMap[key] = obj;
    }
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

void Forte::Context::Merge(const Context &other)
{
    FTRACE;
    if (&other == this) return;
    AutoUnlockMutex lock(mLock);
    AutoUnlockMutex otherLock(other.mLock);
    foreach (const ObjectPair &pair, other.mObjectMap)
    {
        mObjectMap[pair.first] = pair.second;
    }
}
void Forte::Context::Dump(void)
{
    Forte::AutoUnlockMutex lock(mLock);
    hlog(HLOG_INFO, "Context contains %lu objects:", Size());
    foreach (const ObjectPair &p, mObjectMap)
    {
        const FString &name(p.first);
        const ObjectPtr &ptr(p.second);
        int count = (int) ptr.use_count();
        // count > 0 will always be at least 2 due to the foreach
        // subtract one to account for that.
        if (count > 0) --count; 
        hlog(HLOG_DEBUG, "[%d] %s", count, name.c_str());
    }    
}
