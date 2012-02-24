#ifndef __Forte__Context_h__
#define __Forte__Context_h__

#include "Exception.h"
#include "LogManager.h"
#include "Object.h"
#include "AutoMutex.h"
#include <boost/shared_ptr.hpp>

// TODO: How to set certain keys as read-only?

// Macro to allow easy scoping of objects out of the context
#define CGET(key, type, name)                                   \
    shared_ptr<type> name ## _Ptr = mContext.Get<type>(key);    \
    type __attribute__((unused)) &name(* name ## _Ptr)

#define CGETPTR(key, type, name)                                \
    shared_ptr<type> name = mContext.Get<type>(key);

#define CNEW(key, type, args...)                                \
    mContext.Set(key, shared_ptr<type>(new type( args )))

#define CGETNEW(key, type, name, args...)               \
    shared_ptr<type> name ## _Ptr(new type( args ));    \
    mContext.Set(key, name ## _Ptr);                    \
    type __attribute__((unused)) &name(* name ## _Ptr)

#define CGETNEWPTR(key, type, name, args...)                    \
    shared_ptr<type> name(new type( args ));                    \
    mContext.Set(key, name);

#define CSET(key, type, obj)                    \
    mContext.Set(key, shared_ptr<type>( obj ))

namespace Forte
{
    EXCEPTION_CLASS(EContext);
    EXCEPTION_SUBCLASS2(EContext, EContextInvalidKey, "Invalid Key");
    EXCEPTION_SUBCLASS2(EContext, EContextEmptyPointer, "Empty Pointer");
    EXCEPTION_SUBCLASS2(EContext, EContextTypeMismatch, "Context Type Mismatch");

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
     *
     * \code
     Forte::Context context;
     context.Detach("key");
     \endcode
    **/
    class Context : public Object
    {
    public:
        Context();

        virtual ~Context();

        /**
         * Detach() will make a copy of an object within this Context
         * instance, such that if this Context object had been copied
         * from another Context object, the object referenced by the
         * given key will no longer reference the orginal object, but
         * instead a separate local instance.
         **/
        void Detach(const char *key) { throw_exception(EUnimplemented()); }

        /**
         * Get() retrieves a reference counted pointer to an object
         * from the Context.  If the object does not exist, one can
         * be automatically created using an appropriate factory.
         **/
        ObjectPtr Get(const char *key) const;

        /**
         * Get() retrieves a reference counted pointer to a typed object
         * from the Context.  If the object does not exist, one can
         * be automatically created using an appropriate factory.
         **/
        template <typename ValueType>
        boost::shared_ptr<ValueType> Get(const char *key) const {
            ObjectMap::const_iterator i;
            Forte::AutoUnlockMutex lock(mLock);
            if ((i = mObjectMap.find(key)) == mObjectMap.end())
                throw_exception(EContextInvalidKey(key));
            boost::shared_ptr<ValueType> ptr(
                boost::dynamic_pointer_cast<ValueType>((*i).second));
            if (!ptr)
            {
                hlog_and_throw(HLOG_ERR, EContextTypeMismatch(FStringFC(), "Invalid type for '%s': asked for %s, but %s was found", key, typeid(ValueType).name(), typeid((*i).second).name()));
            }
            return ptr;
        }


        /**
         * Set() stores a reference to an object in the Context.  Any
         * previous entry with the same key will be replaced.
         **/
        void Set(const char *key, ObjectPtr obj);

        /**
         * Create() allocates and sets a key to a specified object. TODO
         **/

        /**
         * Remove() will remove a single object from the Context.
         **/
        void Remove(const char *key);

        /**
         * Clear() will remove all references from the Context.
         **/
        void Clear(void);

        /**
         * Merge() will merge all keys from the 'other' Context into
         * this one.  Duplicate keys will be replaced with those from 'other'.
         **/
        void Merge(const Context &other);

        size_t Size(void) { return mObjectMap.size(); }
        void Dump(void);

    protected:
        mutable Forte::Mutex mLock;
        ObjectMap mObjectMap;
    };
    typedef boost::shared_ptr<Context> ContextPtr;
};

#endif
