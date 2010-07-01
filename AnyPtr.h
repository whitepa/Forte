#ifndef __any_ptr_h_
#define __any_ptr_h_
#include "Exception.h"

namespace Forte
{
    EXCEPTION_CLASS(EAnyPtr);
    EXCEPTION_SUBCLASS(EAnyPtr, ETypeMismatch);
    EXCEPTION_SUBCLASS(EAnyPtr, EInvalidReference);

    class AnyPtr
    {
        /**
         * AnyPtr is a pointer to any copy-constructible type.
         * Pointers to appropriate Delete() and Clone() methods are
         * stored such that they may be called later, when type
         * information may not be available.
         **/
    public:
        // forward declarations
        template <class T>
        struct functions;

        template <class T>
        AnyPtr(T* x)
            : mAny(x), mTable(&functions<T>::table)
            {}

        AnyPtr()
            : mAny(NULL), mTable(NULL)
            {}

        ~AnyPtr()
            { if (mAny) Delete(); }

        template <class T>
        AnyPtr & operator=(T* x)
            { mAny = x; mTable = &functions<T>::table; return *this; }

        void Delete() {
            assert(mAny != NULL);
            mTable->Delete(mAny);
            mAny = NULL;
        }
        AnyPtr Clone() const {
            AnyPtr o(*this);
            o.mAny = mTable->Clone(mAny);
            return o;
        }
        const std::type_info& GetType() const {
            return mTable->GetType(mAny);
        }
        template<typename ValueType>
        ValueType* PtrCast() {
            if (!(typeid(ValueType) == GetType()))
                throw ETypeMismatch();
            return static_cast<ValueType*>(mAny);
        }

        template<typename ValueType>
        ValueType& RefCast() {
            if (!(typeid(ValueType) == GetType()))
                throw ETypeMismatch();
            if (!mAny)
                throw EInvalidReference();
            return *static_cast<ValueType*>(mAny);
        }

        template<typename ValueType>
        ValueType& RefCast2() {
            if (!mAny)
                throw EInvalidReference();
            ValueType *vtPtr = dynamic_cast<ValueType*>(mAny);
            if (vtPtr == NULL)
                throw ETypeMismatch();
            else
                return *vtPtr;
        }

        // Function table type
        // order is important, must match all other lists of functions
        struct table {
            void (*Delete)(void*);
            const std::type_info& (*GetType)(void*);
            void* (*Clone)(void*);
         };

        // For a given referenced type T, generates functions for the
        // function table and a static instance of the table.
        template<class T>
        struct functions
        {
            static typename AnyPtr::table table;
            static void Delete(void* p) {
                delete static_cast<T*>(p);
            }
            static const std::type_info& GetType(void* p) {
                return typeid(T);
            }
            static void* Clone(void* p) {
                return new T(*static_cast<T*>(p));
            }
        };
    private:
        void* mAny;
        table* mTable;
    };

    template<class T>
    typename AnyPtr::table
    AnyPtr::functions<T>::table = {
        &AnyPtr::template functions<T>::Delete
        ,&AnyPtr::template functions<T>::GetType
        ,&AnyPtr::template functions<T>::Clone
    };
};

#endif
