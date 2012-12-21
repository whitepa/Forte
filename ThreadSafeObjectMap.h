#ifndef __Forte__ThreadSafeObjectMap_h__
#define __Forte__ThreadSafeObjectMap_h__

#include "Object.h"
#include "AutoMutex.h"

namespace Forte
{
    /**
     * This class implements a 'context' which may be passed into
     * various constructors.  The context contains references to other
     * objects and/or configuration data needed by the object being
     * constructed, and may be used and referenced throughout the
     * lifetime of the object.  Boost shared_ptr objects are used
     * extensively, as this provides a simple reference counting
     * mechanism for objects contained in the context.  Contexts may
     * also be copied, at which time setting an object in the copy
     * will not affect the original context.
    **/
    class ThreadSafeObjectMap  // final
    {
    public:
        /**
         * IsEmpty() returns true if the map is empty, false otherwise.
         **/
         bool IsEmpty() const;

        /**
         * IsEmpty_Unsafe() is like IsEmpty(), but isn't thread-safe.
         **/
         bool IsEmpty_Unsafe() const { return mObjectMap.empty(); }

        /**
         * Get() retrieves a reference counted pointer to an object
         * from the Context.  If the object does not exist, one can
         * be automatically created using an appropriate factory.
         **/
        ObjectPtr Get(const char *key) const;

        /**
         * Set() stores a reference to an object in the Context.  Any
         * previous entry with the same key will be replaced.
         **/
        void Set(const char *key, ObjectPtr obj);

        /**
         * Remove() will remove a single object from the Context.
         **/
        void Remove(const char *key);

        /**
         * Clear() will remove all references from the Context.
         **/
        void Clear(void);

        void Dump(void);

    private:
        mutable Forte::Mutex mLock;
        ObjectMap mObjectMap;
    };
};

#endif
