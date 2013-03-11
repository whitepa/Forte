#ifndef FORTE_NO_OPENSSL
#include "SecureString.h"
#include "Base64.h"
#include "Exception.h"
#include <openssl/pem.h>
#include <openssl/err.h>

using namespace Forte;

KeyBuffer::KeyBuffer()
{
    if ((mBuf = BIO_new(BIO_s_mem()))==NULL)
        throw ESecureString("KeyBuffer(): failed to allocate buffer");
}

KeyBuffer::KeyBuffer(const FString &in)
{
    if ((mBuf = BIO_new(BIO_s_mem()))==NULL)
        throw ESecureString("KeyBuffer(): failed to allocate buffer");
//    BIO_write(mBuf,(void *)in.c_str(), in.length());
    BIO_puts(mBuf, in.c_str());
//    if ((mBuf = BIO_new_mem_buf(const_cast<void *>((const void *)in.c_str()), in.length()))==NULL)
//        throw ESecureString("KeyBuffer(): failed to allocate buffer");
}

KeyBuffer::~KeyBuffer()
{
    BIO_free(mBuf);
}

//////////////////

PublicKey::PublicKey(const KeyBuffer &keybuf, const char *passphrase)
{
//    char *ptr = NULL;
//    BIO_get_mem_data(keybuf.mBuf, &ptr);
//    cout << "about to use BIO buffer: " << ptr << endl;
//    cout << "BIO_tell(): " << BIO_tell(keybuf.mBuf) << endl;
//    cout << "BIO_reset(): " << BIO_reset(keybuf.mBuf) << endl;
//    cout << "BIO_tell(): " << BIO_tell(keybuf.mBuf) << endl;
    if ((mKey = PEM_read_bio_RSA_PUBKEY(keybuf.mBuf, NULL, NULL,
                                        const_cast<void *>(reinterpret_cast<const void*>(passphrase))))==NULL)
    {
        ERR_load_crypto_strings();
//        ERR_print_errors_fp(stderr);
        throw ESecureString(FStringFC(),
                         "failed to load public key: %s",
                         ERR_error_string(ERR_get_error(), NULL));
    }
}

PublicKey::~PublicKey()
{

}

PrivateKey::PrivateKey(const KeyBuffer &keybuf, const char *passphrase)
{
//    char *ptr = NULL;
//    BIO_get_mem_data(keybuf.mBuf, &ptr);
//    cout << "PrivateKey about to use BIO buffer: " << ptr << endl;
//    BIO_reset(keybuf.mBuf);
    if ((mKey = PEM_read_bio_RSAPrivateKey(keybuf.mBuf, NULL, NULL,
                                           const_cast<void *>(reinterpret_cast<const void*>(passphrase))))==NULL)
//    if (PEM_read_bio_PrivateKey(keybuf.mBuf, NULL, NULL,
//                                const_cast<void *>((const void*)passphrase))==NULL)
    {
        ERR_load_crypto_strings();
//        ERR_print_errors_fp(stderr);
        throw ESecureString(FStringFC(),
                         "failed to load private key: %s",
                         ERR_error_string(ERR_get_error(), NULL));
    }
}

PrivateKey::~PrivateKey()
{

}

/////////////////////

RSAString::RSAString(const FString &plaintext, PublicKey &key)
{
    size_t size = RSA_size(key.mKey);
    // length must be less than RSA_size() - 11 (per RSA_public_encrypt(3))
    if (plaintext.length() + 1 > size - 11)
        throw ESecureString(FStringFC(), "plaintext is too long for key; max is %u bytes",
                            static_cast<unsigned>(size - 11));
    unsigned char *ciphertext = new unsigned char[size];
    memset(ciphertext, 0, size);
    if (RSA_public_encrypt(plaintext.length()+1,
                           reinterpret_cast<const unsigned char *>(plaintext.c_str()),
                           ciphertext, key.mKey, RSA_PKCS1_PADDING) == -1)
    {
        delete [] ciphertext;
        ERR_load_crypto_strings();
        throw ESecureString(FStringFC(), "RSAString: Encryption failed: %s",
                         ERR_error_string(ERR_get_error(), NULL));
    }
    // convert to text
    Base64::Encode(reinterpret_cast<char *>(ciphertext), size, mCiphertext);
    memset(ciphertext, 0, size);
    delete [] ciphertext;
}
RSAString::~RSAString()
{

}
void RSAString::GetPlainText(FString &plaintext/*OUT*/, PrivateKey &key)
{
    size_t size = RSA_size(key.mKey);
    unsigned char *plain = new unsigned char[size];
//    cerr << "RSA size is " << size << endl;
    // convert to binary
    FString binCiphertext;
    Base64::Decode(mCiphertext, binCiphertext);
//    cerr << "binCiphertext size is " << binCiphertext.length() << endl;
    int plainsize = 0;
    if ((plainsize = RSA_private_decrypt(binCiphertext.length(),
                                         reinterpret_cast<const unsigned char *>(binCiphertext.data()),
                                         plain, key.mKey, RSA_PKCS1_PADDING))<0)
    {
        delete [] plain;
        ERR_load_crypto_strings();
        ERR_print_errors_fp(stderr);
        throw ESecureString(FStringFC(), "RSAString: Decryption failed: %s",
                         ERR_error_string(ERR_get_error(), NULL));
    }
    else if (plainsize > static_cast<int>(size))
    {
        delete [] plain;
        throw ESecureString("plainsize > RSA_size()");
    }
    // don't count the last char if it's null when assigning to the FString
    if (plainsize > 0 && plain[plainsize - 1] == 0)
        plainsize--;
    plaintext.assign(reinterpret_cast<const char *>(plain), plainsize);
    delete [] plain;
}
#endif
