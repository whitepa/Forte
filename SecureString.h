#ifndef __Secure_String_h_
#define __Secure_String_h_
#ifndef FORTE_NO_OPENSSL
#include "FString.h"
#include <openssl/bio.h>
#include <openssl/rsa.h>

EXCEPTION_SUBCLASS(Exception, ForteSecureStringException);

class PublicKey;
class PrivateKey;
class RSAString;

class KeyBuffer {
    friend class PublicKey;
    friend class PrivateKey;
public:
    KeyBuffer();
    KeyBuffer(const FString &in);
    virtual ~KeyBuffer();
protected:
    BIO *mBuf;
};

class PublicKey {
    friend class RSAString;
public:
    PublicKey(const KeyBuffer &keybuf, const char *passphrase);
    virtual ~PublicKey();
protected:
    RSA *mKey;
};

class PrivateKey {
    friend class RSAString;
public:
    PrivateKey(const KeyBuffer &keybuf, const char *passphrase);
    virtual ~PrivateKey();
protected:
    RSA *mKey;
};

class RSAString {
public:
    RSAString(const FString &plaintext, PublicKey &key);
    RSAString(const FString &ciphertext) : mCiphertext(ciphertext) {};
    virtual ~RSAString();

    void getPlaintext(FString &plaintext/*OUT*/, PrivateKey &key);

//    inline operator const FString &() const { return mCiphertext; }
    inline operator const char *() const { return mCiphertext.c_str(); }

private:
    FString mCiphertext;
};

#endif
#endif
