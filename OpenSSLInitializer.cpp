#include "Forte.h"
#ifndef FORTE_NO_OPENSSL
#include "OpenSSLInitializer.h"
#include "openssl/crypto.h"
#include "openssl/evp.h"

using namespace Forte;

static Mutex * sOpenSSLMutexArray;
struct CRYPTO_dynlock_value { Mutex mutex; };


void OpenSSLInitializer_lockingCallback(int mode, int n, const char *file, int line) {

    if (mode & CRYPTO_LOCK) {
        sOpenSSLMutexArray[n].Lock();
    } else {
        sOpenSSLMutexArray[n].Unlock();
    }
}

unsigned long OpenSSLInitializer_threadIdCallback() {
    return (unsigned long)pthread_self();
}

struct CRYPTO_dynlock_value * 
OpenSSLInitializer_dynlockCreateCallback(const char *file, int line) {
    return new CRYPTO_dynlock_value;
}

void OpenSSLInitializer_dynlockLockingCallback(int mode,
                                                struct CRYPTO_dynlock_value *l,
                                                const char *file, int line) {
    if (mode & CRYPTO_LOCK) {
        l->mutex.Lock();
    } else {
        l->mutex.Unlock();
    }
}

void OpenSSLInitializer_dynlockDestroyCallback(struct CRYPTO_dynlock_value *l,
                                                const char *file, int line) {
    if (l != NULL) delete l;
}

/// Properly initializes the OpenSSL library for a threaded environment. 
/// Declare one of these in main() if you are using OpenSSL in a multithreaded program.
OpenSSLInitializer::OpenSSLInitializer() {

   sOpenSSLMutexArray = new Mutex[CRYPTO_num_locks()];
   CRYPTO_set_id_callback(OpenSSLInitializer_threadIdCallback);
   CRYPTO_set_locking_callback(OpenSSLInitializer_lockingCallback);
   CRYPTO_set_dynlock_create_callback(
       OpenSSLInitializer_dynlockCreateCallback);
   CRYPTO_set_dynlock_lock_callback(
       OpenSSLInitializer_dynlockLockingCallback);
   CRYPTO_set_dynlock_destroy_callback(
       OpenSSLInitializer_dynlockDestroyCallback);

   OpenSSL_add_all_algorithms();
   OpenSSL_add_all_ciphers();
   OpenSSL_add_all_digests();
}

OpenSSLInitializer::~OpenSSLInitializer() {
   CRYPTO_set_id_callback(NULL);
   CRYPTO_set_locking_callback(NULL);
   CRYPTO_set_dynlock_create_callback(NULL);
   CRYPTO_set_dynlock_lock_callback(NULL);
   CRYPTO_set_dynlock_destroy_callback(NULL);

   delete [] sOpenSSLMutexArray;
   sOpenSSLMutexArray = NULL;

   EVP_cleanup();
}
#endif
