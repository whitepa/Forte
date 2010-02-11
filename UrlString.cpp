#include "UrlString.h"
#include "Forte.h"


FString CUrlString::sUnsafe = "%=\"<>\\^[]`+$,@:;/!#?&'";

FString & CUrlString::encode(FString &encoded, const FString &decoded)
{
    int len = decoded.length();
    encoded.reserve(len * 2); // should be plenty for most cases
    int i = 0;
    while (i < len)
    {
        char c = decoded[i];
        if (c < 33 || c > 123 || sUnsafe.find(c, 0) != string::npos)
            // character is unsafe
            encoded += FString(FStringFC(), "%%%02X", c);
        else
            // character is safe
            encoded += c;
        ++i;
    }
    return encoded;
}

// following adapted from:
// http://www.bjnet.edu.cn/tech/book/seucgi/ch22.htm
FString & CUrlString::decode(FString &decoded, const FString &encoded)
{
    decoded.reserve(encoded.size());
    // loop through looking for escapes
    const unsigned int elen = encoded.length();
    for (unsigned int i = 0; i < elen; ++i)
    {
        if (encoded[i]=='+')
            decoded+=' ';
        else if (encoded[i]=='%') {
            // A percent sign followed by two hex digits means
            // that the digits represent an escaped character.
            // We must decode it.
            if (i+2 < elen && isxdigit(encoded[i+1]) && isxdigit(encoded[i+2]))
            {
                decoded += charFromHexPair(encoded[i+1],encoded[i+2]);
                i+=2;
            }
        } else {
            decoded+=encoded[i];
        }
    }
    return decoded;
}

// a subroutine that unescapes escaped characters:
char CUrlString::charFromHexPair(char hi, char lo)
{
    // high nibble
    if ('0' <= hi && hi <= '9') {
        hi -= '0';
    } else
    if ('a' <= hi && hi <= 'f') {
        hi -= ('a'-10);
    } else
    if ('A' <= hi && hi <= 'F') {
        hi -= ('A'-10);
    }
    // low nibble
    if ('0' <= lo && lo <= '9') {
        lo -= '0';
    } else
    if ('a' <= lo && lo <= 'f') {
        lo -= ('a'-10);
    } else
    if ('A' <= lo && lo <= 'F') {
        lo -= ('A'-10);
    }
    return lo + (16 * hi);
}

