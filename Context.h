#ifndef __Forte__Context_h__
#define __Forte__Context_h__

#include "AnyPtr.h"
#include <boost/shared_ptr.hpp>

/**
 * TODO: This CAST() macro is currently needed to convert the
 * ObjectPtr type (a shared_ptr<AnyPtr>) into whatever type is
 * actually needed.  If there is a way to return a shared_ptr to the
 * specific type while updating the correct reference count (currently
 * in the shared_ptr<AnyPtr> stored within this Context) then that
 * method should be used.  This will probably require a shared_any_ptr
 * type.
 **/
#define CAST(type, arg) ((*arg).RefCast<type>())

namespace Forte
{
    EXCEPTION_CLASS(EContext);
    EXCEPTION_SUBCLASS(EContext, EInvalidKey);

    /**
     * This class implements a 'context' which may be passed into
     * various constructors.  The context contains references to other
     * objects and/or configuration data needed by the object being
     * constructed, and may be used and referenced throughout the
     * lifetime of the object.  Boost shared_ptr objects are used
     * extensively, as this provides a simple reference counting
     * mechanism for objects contained in the context.  Contexts may
     * also be copied, at which time setting an object in the copy
     * will not affect the original context.  TODO: Warn about Getting
     * a reference, then removing an object from the context.  TODO:
     * make this thread safe (locking around map operations)
     **/
    class Context
    {
    public:
        Context() {};
        Context(const Context &other) { throw EUnimplemented(); }
        virtual ~Context() {};

        typedef boost::shared_ptr<AnyPtr> ObjectPtr;

        /**
         * Copy() makes a copy of the requested object, to which any
         * future Get() calls to this context will reference.  
         * TODO:
         *   not entirely sure if this is needed yet.
         **/
//        void Copy(const char *key);

        /**
         * GetRef() retrieves a reference to an object from the
         * Context.  If the object does not exists, one can be
         * automatically created using an appropriate factory.
         **/
        template <typename ValueType>
        ValueType& GetRef(const char *key) const {
            ObjectMap::const_iterator i;
            if ((i = mObjectMap.find(key)) == mObjectMap.end())
                // TODO: use a factory to create one?
                throw EInvalidKey();
            return (*((*i).second)).RefCast<ValueType>();
        }

        /**
         * Get() retrieves a reference counted pointer to an object
         * from the Context.  If the object does not exists, one can
         * be automatically created using an appropriate factory.
         **/
        ObjectPtr Get(const char *key) const {
            ObjectMap::const_iterator i;
            if ((i = mObjectMap.find(key)) == mObjectMap.end())
                // TODO: use a factory to create one?
                throw EInvalidKey();
            return (*i).second;
        }
        
        /**
         * Set() stores a reference to an object in the Context.
         **/
        template<typename ValueType>
        void Set(const char *key, ValueType *ref) {
            mObjectMap[key] = shared_ptr<AnyPtr>(new AnyPtr(ref));
        }

        /**
         * Create() allocates and sets a key to a specified object. TODO
         **/

        /**
         * Remove() will remove a single object from the Context.
         **/
        void Remove(const char *key) {
            mObjectMap.erase(key);
        }

        /**
         * Clear() will remove all references from the Context.
         **/
        void Clear(void) {
            mObjectMap.clear();
        }

        /**
         * Ideally, we would use some kind of shared_any_ptr type, but
         * no such thing exists (yet), so we use shared_ptr<AnyPtr>.
         **/
    protected:
        typedef std::map<FString, ObjectPtr> ObjectMap;
        ObjectMap mObjectMap;
    };
};

#endif
