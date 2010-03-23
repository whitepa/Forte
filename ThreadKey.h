#ifndef __ThreadKey_h_
#define __ThreadKey_h_

namespace Forte
{
    class ThreadKey {
    public:
        inline ThreadKey(void (*destructor)(void *) = NULL)
            { pthread_key_create(&mKey, destructor); }
        inline ~ThreadKey()
            { pthread_key_delete(mKey); }

        void *get(void)  __attribute__ ((no_instrument_function));
        void set(const void *value) __attribute__ ((no_instrument_function));

    protected:
        pthread_key_t mKey;
    };
};
#endif
