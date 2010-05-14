#include "Forte.h"

void * ThreadKey::Get(void) { return pthread_getspecific(mKey); }

void ThreadKey::Set(const void *value) { pthread_setspecific(mKey, value); }
