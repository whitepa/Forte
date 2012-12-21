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
    if (!mObjectMap.IsEmpty_Unsafe())
    {
        hlog(HLOG_DEBUG, "Objects Remain in Context at deletion:");
        Dump();
    }
    Clear();
}

Forte::ObjectPtr Forte::Context::Get(const char *key) const
{
    ObjectPtr result = mObjectMap.Get(key);
    if (result.get() == NULL)
    {
        // use a factory to create one?  NO.  Objects must be
        // explicitly created.  This avoids potential problems if
        // destructors decide to try to access context objects after
        // they have been removed.
        throw_exception(EContextInvalidKey(key));
    }
    return result;
}

void Forte::Context::Set(const char *key, ObjectPtr obj)
{
    if (!obj)
    {
        // disallow setting to an empty pointer
        throw_exception(EContextEmptyPointer(key));
    }
    mObjectMap.Set(key, obj);
}

void Forte::Context::Remove(const char *key)
{
    mObjectMap.Remove(key);
}

void Forte::Context::Clear(void)
{
   mObjectMap.Clear();
}

void Forte::Context::Merge(const Context &other)
{
    FTRACE;
    if (&other == this) return;
    const bool meFirst = this < &other;
    AutoUnlockMutex lockOne(meFirst ? mLock : other.mLock);
    AutoUnlockMutex lockTwo(meFirst ? other.mLock : mLock);
    foreach (const ObjectPair &pair, other.mObjectMap)
    {
        mObjectMap[pair.first] = pair.second;
    }
}
void Forte::Context::Dump(void)
{
    mObjectMap.Dump();
}
