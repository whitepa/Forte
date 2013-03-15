#include "ThreadSafeObjectMap.h"

#include "Foreach.h"
#include "LogManager.h"

bool Forte::ThreadSafeObjectMap::IsEmpty() const
{
    Forte::AutoUnlockMutex lock(mLock);
    return IsEmpty_Unsafe();
}

Forte::ObjectPtr Forte::ThreadSafeObjectMap::Get(const char *key) const
{
    ObjectMap::const_iterator i;
    Forte::AutoUnlockMutex lock(mLock);
    if ((i = mObjectMap.find(key)) == mObjectMap.end())
    {
        // use a factory to create one?  NO.  Objects must be
        // explicitly created.  This avoids potential problems if
        // destructors decide to try to access context objects after
        // they have been removed.
        return ObjectPtr();
    }
    return (*i).second;
}

void Forte::ThreadSafeObjectMap::Set(const char *key, ObjectPtr obj)
{
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

void Forte::ThreadSafeObjectMap::Remove(const char *key)
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

void Forte::ThreadSafeObjectMap::Clear(void)
{
    // we must not cause object deletions while holding the lock
    ObjectMap localCopy;
    {
        Forte::AutoUnlockMutex lock(mLock);
        mObjectMap.swap(localCopy);
    }
}

void Forte::ThreadSafeObjectMap::Dump(void)
{
    Forte::AutoUnlockMutex lock(mLock);
    hlog(HLOG_DEBUG, "ThreadSafeObjectMap contains %lu objects:", mObjectMap.size());
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
