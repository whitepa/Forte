#ifndef __ThreadKey_h_
#define __ThreadKey_h_

#include <pthread.h>
#include "Exception.h"

namespace Forte{

EXCEPTION_CLASS(EThreadKey);
// -----------------------------------------------------------------------------

    class ThreadKey {
    public:
        ThreadKey(void (*destructor)(void *) = NULL)
        { pthread_key_create(&mKey, destructor); }
        virtual ~ThreadKey()
        { pthread_key_delete(mKey); }

        void *Get(void)  __attribute__ ((no_instrument_function));
        void Set(const void *value) __attribute__ ((no_instrument_function));

    protected:
        pthread_key_t mKey;

    private:
        ThreadKey(const ThreadKey &other)
            { throw EThreadKey(); }
        const ThreadKey &operator=(const ThreadKey &rhs)
            { throw EThreadKey(); return *this; }
    };

}; /* namespace Forte{} */
// -----------------------------------------------------------------------------

#endif
