#ifndef FORTE_NO_OPENSSL
#include "SecureEnvelope.h"

using namespace Forte;

FString & SecureEnvelopeEncoder::Encode(const FString &accountNumber, 
                                        unsigned int numClear, 
                                        FString &out)
{
    if (accountNumber.length() < numClear || accountNumber.find_first_of('$') != std::string::npos)
        throw ForteSecureEnvelopeEncoderException("invalid account number");
    out.clear();
    out.reserve(accountNumber.length() + 130);
    int obscuredChars = accountNumber.length() - numClear;
    out.append(obscuredChars, 'X');
    out.append(accountNumber.Right(numClear));
    FString toEncrypt; // build the string we are going to encrypt
    // We encrypt a scrambled version of the card number, so we don't make it easy to brute force.
    // start with random data the same length as our obscured account numbers:
    toEncrypt.append(Random::GetRandomData(obscuredChars));
    for (int i = 0; i < obscuredChars; ++i)
        toEncrypt.append(1, accountNumber[i] ^ toEncrypt[i]); // bitwise XOR with the random data
    RSAString encrypted(toEncrypt, mPublicKey);
    out.append(1, '$');
    out.append(encrypted);
    return out;
}

FString & SecureEnvelopeDecoder::Decode(const FString &encoded, FString &out)
{
    out.clear();
    size_t obscuredChars = encoded.find_first_not_of('X');
    size_t dollarPos = encoded.find_first_of('$', obscuredChars);
    if (obscuredChars == 0 && dollarPos == std::string::npos)
    {
        // not encoded
        out = encoded;
        return out;
    }
    RSAString encrypted(encoded.Mid(dollarPos + 1));
//    cout << "ENCRYPTED: " << encrypted << endl;
    FString decrypted;
    // decrypt it
    encrypted.GetPlainText(decrypted, mPrivateKey);
    // descramble it
    for (size_t i = 0; i < obscuredChars; ++i)
        out.append(1, decrypted[i] ^ decrypted[i + obscuredChars]);
    // append the final cleartext portion
    out.append(encoded.Mid(obscuredChars, dollarPos - obscuredChars));
    return out;
}

FString & SecureEnvelopeDecoder::Obscured(const FString &encoded, FString &out)
{
    size_t dollarPos;
    if ((dollarPos = encoded.find_first_of('$')) == std::string::npos)
        throw ForteSecureEnvelopeEncoderException("unable to decode data");
    out.assign(encoded.Left(dollarPos));
    return out;
}

bool SecureEnvelopeDecoder::IsEncoded(const std::string &data)
{
    // XXX improve this!
    size_t obscuredChars = data.find_first_not_of('X');
    size_t dollarPos = data.find_first_of('$', obscuredChars);
    if (obscuredChars != std::string::npos &&
        dollarPos != std::string::npos &&
        dollarPos >= obscuredChars)
        return true;
    else
        return false;
}
#endif
