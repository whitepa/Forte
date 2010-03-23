#include "Forte.h"

void * ThreadKey::get(void) { return pthread_getspecific(mKey); }

void ThreadKey::set(const void *value) { pthread_setspecific(mKey, value); }
