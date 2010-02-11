#include "Forte.h"

void * CThreadKey::get(void) { return pthread_getspecific(mKey); }

void CThreadKey::set(const void *value) { pthread_setspecific(mKey, value); }
