#ifndef __Secure_String_h_
#define __Secure_String_h_
#ifndef FORTE_NO_OPENSSL
#include "FString.h"
#include <openssl/bio.h>
#include <openssl/rsa.h>

EXCEPTION_SUBCLASS(CForteException, CForteSecureStringException);

class CPublicKey;
class CPrivateKey;
class CRSAString;

class CKeyBuffer {
    friend class CPublicKey;
    friend class CPrivateKey;
public:
    CKeyBuffer();
    CKeyBuffer(const FString &in);
    virtual ~CKeyBuffer();
protected:
    BIO *mBuf;
};

class CPublicKey {
    friend class CRSAString;
public:
    CPublicKey(const CKeyBuffer &keybuf, const char *passphrase);
    virtual ~CPublicKey();
protected:
    RSA *mKey;
};

class CPrivateKey {
    friend class CRSAString;
public:
    CPrivateKey(const CKeyBuffer &keybuf, const char *passphrase);
    virtual ~CPrivateKey();
protected:
    RSA *mKey;
};

class CRSAString {
public:
    CRSAString(const FString &plaintext, CPublicKey &key);
    CRSAString(const FString &ciphertext) : mCiphertext(ciphertext) {};
    virtual ~CRSAString();

    void getPlaintext(FString &plaintext/*OUT*/, CPrivateKey &key);

//    inline operator const FString &() const { return mCiphertext; }
    inline operator const char *() const { return mCiphertext.c_str(); }

private:
    FString mCiphertext;
};

#endif
#endif
