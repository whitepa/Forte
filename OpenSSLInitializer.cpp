#include "Forte.h"
#ifndef FORTE_NO_OPENSSL
#include "OpenSSLInitializer.h"
#include "openssl/crypto.h"
#include "openssl/evp.h"

static Mutex * sOpenSSLMutexArray;
struct CRYPTO_dynlock_value { Mutex mutex; };


void COpenSSLInitializer_lockingCallback(int mode, int n, const char *file, int line) {

    if (mode & CRYPTO_LOCK) {
        sOpenSSLMutexArray[n].lock();
    } else {
        sOpenSSLMutexArray[n].unlock();
    }
}

unsigned long COpenSSLInitializer_threadIdCallback() {
    return (unsigned long)pthread_self();
}

struct CRYPTO_dynlock_value * 
COpenSSLInitializer_dynlockCreateCallback(const char *file, int line) {
    return new CRYPTO_dynlock_value;
}

void COpenSSLInitializer_dynlockLockingCallback(int mode,
                                                struct CRYPTO_dynlock_value *l,
                                                const char *file, int line) {
    if (mode & CRYPTO_LOCK) {
        l->mutex.lock();
    } else {
        l->mutex.unlock();
    }
}

void COpenSSLInitializer_dynlockDestroyCallback(struct CRYPTO_dynlock_value *l,
                                                const char *file, int line) {
    if (l != NULL) delete l;
}

/// Properly initializes the OpenSSL library for a threaded environment. 
/// Declare one of these in main() if you are using OpenSSL in a multithreaded program.
COpenSSLInitializer::COpenSSLInitializer() {

   sOpenSSLMutexArray = new Mutex[CRYPTO_num_locks()];
   CRYPTO_set_id_callback(COpenSSLInitializer_threadIdCallback);
   CRYPTO_set_locking_callback(COpenSSLInitializer_lockingCallback);
   CRYPTO_set_dynlock_create_callback(
       COpenSSLInitializer_dynlockCreateCallback);
   CRYPTO_set_dynlock_lock_callback(
       COpenSSLInitializer_dynlockLockingCallback);
   CRYPTO_set_dynlock_destroy_callback(
       COpenSSLInitializer_dynlockDestroyCallback);

   OpenSSL_add_all_algorithms();
   OpenSSL_add_all_ciphers();
   OpenSSL_add_all_digests();
}

COpenSSLInitializer::~COpenSSLInitializer() {
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
