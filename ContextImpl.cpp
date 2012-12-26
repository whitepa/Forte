#include "ContextImpl.h"
#include "FTrace.h"
#include "Foreach.h"

Forte::ContextImpl::ContextImpl()
{
    FTRACE;
}

Forte::ContextImpl::~ContextImpl()
{
    FTRACE;
    if (!mObjectMap.IsEmpty_Unsafe())
    {
        hlog(HLOG_DEBUG, "Objects Remain in ContextImpl at deletion:");
        Dump();
    }
    Clear();
}

Forte::ObjectPtr Forte::ContextImpl::Get(const char *key) const
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

void Forte::ContextImpl::Set(const char *key, ObjectPtr obj)
{
    if (!obj)
    {
        // disallow setting to an empty pointer
        throw_exception(EContextEmptyPointer(key));
    }
    mObjectMap.Set(key, obj);
}

void Forte::ContextImpl::Remove(const char *key)
{
    mObjectMap.Remove(key);
}

void Forte::ContextImpl::Clear(void)
{
   mObjectMap.Clear();
}

void Forte::ContextImpl::Dump(void)
{
    mObjectMap.Dump();
}
