#include "Forte.h"

using namespace Forte;

void * ThreadKey::Get(void) { return pthread_getspecific(mKey); }

void ThreadKey::Set(const void *value) { pthread_setspecific(mKey, value); }
