#ifndef __Forte__Context_h__
#define __Forte__Context_h__

#include "Object.h"
#include <boost/shared_ptr.hpp>

// TODO: thread safety!
// TODO: How to set certain keys as read-only?

namespace Forte
{
    EXCEPTION_CLASS(EContext);
    EXCEPTION_SUBCLASS(EContext, EInvalidKey);
    EXCEPTION_SUBCLASS(EContext, EContextTypeMismatch);

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

        /**
         * Detach() will make a copy of an object within this Context
         * instance, such that if this Context object had been copied
         * from another Context object, the object referenced by the
         * given key will no longer reference the orginal object, but
         * instead a separate local instance.
         **/
        void Detach(const char *key) { throw EUnimplemented(); }

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
                throw EInvalidKey(key);
            shared_ptr<ValueType> ptr(dynamic_pointer_cast<ValueType>((*i).second));
            if (!ptr)
                throw EContextTypeMismatch(); // TODO: include types in error message
            return *ptr;
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
            mObjectMap[key] = ObjectPtr(ref);
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
    protected:
        ObjectMap mObjectMap;
    };
};

#endif
