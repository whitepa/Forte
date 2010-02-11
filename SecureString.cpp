#include "Forte.h"
#ifndef FORTE_NO_OPENSSL
#include "SecureString.h"
#include "Exception.h"
#include <openssl/pem.h>
#include <openssl/err.h>

CKeyBuffer::CKeyBuffer()
{
    if ((mBuf = BIO_new(BIO_s_mem()))==NULL)
        throw CForteSecureStringException("CKeyBuffer(): failed to allocate buffer");
}

CKeyBuffer::CKeyBuffer(const FString &in)
{
    if ((mBuf = BIO_new(BIO_s_mem()))==NULL)
        throw CForteSecureStringException("CKeyBuffer(): failed to allocate buffer");
//    BIO_write(mBuf,(void *)in.c_str(), in.length());
    BIO_puts(mBuf, in.c_str());
//    if ((mBuf = BIO_new_mem_buf(const_cast<void *>((const void *)in.c_str()), in.length()))==NULL)
//        throw CForteSecureStringException("CKeyBuffer(): failed to allocate buffer");
}

CKeyBuffer::~CKeyBuffer()
{
    BIO_free(mBuf);
}

//////////////////

CPublicKey::CPublicKey(const CKeyBuffer &keybuf, const char *passphrase)
{
//    char *ptr = NULL;
//    BIO_get_mem_data(keybuf.mBuf, &ptr);
//    cout << "about to use BIO buffer: " << ptr << endl;
//    cout << "BIO_tell(): " << BIO_tell(keybuf.mBuf) << endl;
//    cout << "BIO_reset(): " << BIO_reset(keybuf.mBuf) << endl;
//    cout << "BIO_tell(): " << BIO_tell(keybuf.mBuf) << endl;
    if ((mKey = PEM_read_bio_RSA_PUBKEY(keybuf.mBuf, NULL, NULL, 
                                          const_cast<void *>((const void*)passphrase)))==NULL)
    {
        ERR_load_crypto_strings();
//        ERR_print_errors_fp(stderr);
        throw CForteSecureStringException(FStringFC(),
                         "failed to load public key: %s",
                         ERR_error_string(ERR_get_error(), NULL));
    }
}

CPublicKey::~CPublicKey()
{

}

CPrivateKey::CPrivateKey(const CKeyBuffer &keybuf, const char *passphrase)
{
//    char *ptr = NULL;
//    BIO_get_mem_data(keybuf.mBuf, &ptr);
//    cout << "CPrivateKey about to use BIO buffer: " << ptr << endl;
//    BIO_reset(keybuf.mBuf);
    if ((mKey = PEM_read_bio_RSAPrivateKey(keybuf.mBuf, NULL, NULL,
                                           const_cast<void *>((const void*)passphrase)))==NULL)
//    if (PEM_read_bio_PrivateKey(keybuf.mBuf, NULL, NULL,
//                                const_cast<void *>((const void*)passphrase))==NULL)
    {
        ERR_load_crypto_strings();
//        ERR_print_errors_fp(stderr);
        throw CForteSecureStringException(FStringFC(),
                         "failed to load private key: %s",
                         ERR_error_string(ERR_get_error(), NULL));
    }
}

CPrivateKey::~CPrivateKey()
{

}

/////////////////////

CRSAString::CRSAString(const FString &plaintext, CPublicKey &key)
{
    size_t size = RSA_size(key.mKey);
    // length must be less than RSA_size() - 11 (per RSA_public_encrypt(3))
    if (plaintext.length() + 1 > size - 11)
        throw CForteSecureStringException(FStringFC(), "plaintext is too long for key; max is %u bytes",
                         (unsigned)(size - 11));
    unsigned char *ciphertext = new unsigned char[size];
    memset(ciphertext, 0, size);
    if (RSA_public_encrypt(plaintext.length()+1, (const unsigned char *)plaintext.c_str(), 
                           ciphertext, key.mKey, RSA_PKCS1_PADDING) == -1)
    {
        delete [] ciphertext;
        ERR_load_crypto_strings();
        throw CForteSecureStringException(FStringFC(), "CRSAString: Encryption failed: %s", 
                         ERR_error_string(ERR_get_error(), NULL));
    }
    // convert to text
    CBase64::Encode((char *)ciphertext, size, mCiphertext);
    memset(ciphertext, 0, size);
    delete [] ciphertext;
}
CRSAString::~CRSAString()
{
    
}
void CRSAString::getPlaintext(FString &plaintext/*OUT*/, CPrivateKey &key)
{
    size_t size = RSA_size(key.mKey);
    unsigned char *plain = new unsigned char[size];
//    cerr << "RSA size is " << size << endl;
    // convert to binary
    FString binCiphertext;
    CBase64::Decode(mCiphertext, binCiphertext);
//    cerr << "binCiphertext size is " << binCiphertext.length() << endl;
    int plainsize = 0;
    if ((plainsize = RSA_private_decrypt(binCiphertext.length(), (const unsigned char *)binCiphertext.data(), 
                                         plain, key.mKey, RSA_PKCS1_PADDING))<0)
    {
        delete [] plain;
        ERR_load_crypto_strings();
        ERR_print_errors_fp(stderr);
        throw CForteSecureStringException(FStringFC(), "CRSAString: Decryption failed: %s",
                         ERR_error_string(ERR_get_error(), NULL));
    }
    else if (plainsize > (int)size)
    {
        delete [] plain;
        throw CForteSecureStringException("plainsize > RSA_size()");
    }
    // don't count the last char if it's null when assigning to the FString
    if (plainsize > 0 && plain[plainsize - 1] == 0)
        plainsize--;
    plaintext.assign((const char *)plain, plainsize);
    delete [] plain;
}
#endif
