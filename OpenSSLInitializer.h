#ifndef  __openssl_initializer_h__
#define  __openssl_initializer_h__
#ifndef FORTE_NO_OPENSSL

/// Properly initializes the OpenSSL library for a threaded environment. 
/// Declare one of these in main() if you are using OpenSSL in a multithreaded program.
namespace Forte
{
    class COpenSSLInitializer {
    public:
        COpenSSLInitializer();
        virtual ~COpenSSLInitializer();
    };
};
#endif
#endif
