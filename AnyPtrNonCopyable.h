#ifndef __any_ptr_noncopyable_h_
#define __any_ptr_noncopyable_h_
#include "Exception.h"

namespace Forte
{
    EXCEPTION_CLASS(EAnyPtrNonCopyable);
    EXCEPTION_SUBCLASS(EAnyPtrNonCopyable, ETypeMismatch);
    EXCEPTION_SUBCLASS(EAnyPtrNonCopyable, EInvalidReference);

    class AnyPtrNonCopyable
    {
        /**
         * AnyPtrNonCopyable is a pointer to any copy-constructible type.
         * Pointers to appropriate Delete() and Clone() methods are
         * stored such that they may be called later, when type
         * information may not be available.
         **/
    public:
        // forward declarations
        template <class T>
        struct functions;

        template <class T>
        AnyPtrNonCopyable(T* x)
            : mAny(x), mTable(&functions<T>::table)
            {}

        AnyPtrNonCopyable()
            : mAny(NULL), mTable(NULL)
            {}

        ~AnyPtrNonCopyable()
            { if (mAny) Delete(); }

        template <class T>
        AnyPtrNonCopyable & operator=(T* x)
            { mAny = x; mTable = &functions<T>::table; return *this; }

        void Delete() {
            assert(mAny != NULL);
            mTable->Delete(mAny);
            mAny = NULL;
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
        ValueType& RefCastUnchecked() {
            // @TODO make a AnyPtr class with a second template
            // parameter: the base class you may want to occasionally
            // cast as.  Until then, we'll live on the edge with this
            // dangerous method.
            if (!mAny)
                throw EInvalidReference();
            return *static_cast<ValueType*>(mAny);
        }

        // Function table type
        // order is important, must match all other lists of functions
        struct table {
            void (*Delete)(void*);
            const std::type_info& (*GetType)(void*);
         };

        // For a given referenced type T, generates functions for the
        // function table and a static instance of the table.
        template<class T>
        struct functions
        {
            static typename AnyPtrNonCopyable::table table;
            static void Delete(void* p) {
                delete static_cast<T*>(p);
            }
            static const std::type_info& GetType(void* p) {
                return typeid(T);
            }
        };
    private:
        void* mAny;
        table* mTable;
    };

    template<class T>
    typename AnyPtrNonCopyable::table
    AnyPtrNonCopyable::functions<T>::table = {
        &AnyPtrNonCopyable::template functions<T>::Delete
        ,&AnyPtrNonCopyable::template functions<T>::GetType
    };
};

#endif
