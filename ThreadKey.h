#ifndef __ThreadKey_h_
#define __ThreadKey_h_

#include "Exception.h"

namespace Forte{

class ThreadKeyException : public Forte::Exception
{
public:
    ThreadKeyException() {}
    virtual ~ThreadKeyException() throw () {}
};

class ThreadKey {
public:
    ThreadKey(void (*destructor)(void *) = NULL)
    { pthread_key_create(&mKey, destructor); }
    virtual ~ThreadKey()
    { pthread_key_delete(mKey); }

private:
    ThreadKey(const ThreadKey &other)
        { throw ThreadKeyException(); }
    const ThreadKey &operator=(const ThreadKey &rhs)
        { throw ThreadKeyException(); return *this; }

public:
    void *Get(void)  __attribute__ ((no_instrument_function));
    void Set(const void *value) __attribute__ ((no_instrument_function));

protected:
    pthread_key_t mKey;
};

}; /* namespace Forte{} */
// -----------------------------------------------------------------------------

#endif
