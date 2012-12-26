#ifndef __Forte__ContextImpl_h__
#define __Forte__ContextImpl_h__

#include "Context.h"
#include "ThreadSafeObjectMap.h"

namespace Forte
{
    class ContextImpl : public Context  // final
    {
    public:
        ContextImpl();

        ~ContextImpl();

        /**
         * Get() retrieves a reference counted pointer to an object
         * from the Context.  If the object does not exist, one can
         * be automatically created using an appropriate factory.
         **/
        ObjectPtr Get(const char *key) const;

        template <typename ValueType>
        boost::shared_ptr<ValueType> Get(const char *key) const
        {
            return Context::Get<ValueType>(key);
        }


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
        ThreadSafeObjectMap mObjectMap;
    };
};

#endif
