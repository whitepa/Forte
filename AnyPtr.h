#ifndef __any_ptr_h_
#define __any_ptr_h_
#include "Forte.h"
// this class can point to any copy-constructible value_type
// and let you clone or delete it safely
class any_ptr
{
public:
    // forward declarations
    template <class T>
    struct functions;


    template <class T>
    any_ptr(T* x)
        : m_a(x), m_t(&functions<T>::table)
        {}

    any_ptr()
        : m_a(NULL), m_t(NULL)
        {}

    ~any_ptr()
        { if (m_a) Delete(); }

    template <class T>
    any_ptr & operator=(T* x)
        { m_a = x; m_t = &functions<T>::table; return *this; }

    void Delete() {
        assert(m_a != NULL);
        m_t->Delete(m_a);
        m_a = NULL;
    }
    any_ptr Clone() const {
        any_ptr o(*this);
        o.m_a = m_t->Clone(m_a);
        return o;
    }
    const std::type_info& GetType() const {
        return m_t->GetType(m_a);
    }
    template<typename ValueType>
    ValueType* PtrCast() {
        if (!(typeid(ValueType) == GetType())) {
            throw CException("Bad Object Cast");
        }
        return static_cast<ValueType*>(m_a);
    }

    template<typename ValueType>
    ValueType* PtrCast2() {
        if (!(typeid(ValueType) == GetType())) {
            throw CException("Bad Object Cast");
        }
        return static_cast<ValueType*>(m_a);
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
        static typename any_ptr::table table;
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
    void* m_a;
    table* m_t;
};

template<class T>
typename any_ptr::table
any_ptr::functions<T>::table = {
    &any_ptr::template functions<T>::Delete
    ,&any_ptr::template functions<T>::GetType
    ,&any_ptr::template functions<T>::Clone
};


#endif
