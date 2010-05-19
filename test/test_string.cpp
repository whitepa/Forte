#include <iostream>
#include <cstdio>
#include "Forte.h"
//#include "FTime.h"
#include "SecureString.h"
#include "SecureEnvelope.h"
#include "teststuff.h"
#include "boost/foreach.hpp"
#define foreach BOOST_FOREACH

int main2(int argc, char * const argv[]);
int main(int argc, char * const argv[])
{
    try
    {
#if !defined(FORTE_NO_BOOST)
        return main2(argc, argv);
#else
        cout << "Forte is not configured with Boost support." << endl;
        return 0;
#endif
    }
    catch (Exception &e)
    {
        cout << "Caught Exception: " << e.What() << endl;
    }
    catch (std::exception &e)
    {
        cout << "Caught Exception: " << e.what() << endl;
    }
    catch (...)
    {
        cout << "Caught unknown type" << endl;
    }
}

#if !defined(FORTE_NO_BOOST)
int main2(int argc, char * const argv[])
{
//    FTime::init("/usr/local/lib/date_time_zonespec.csv");
    OpenSSLInitializer sslInit;
    unsigned int numFailed = 0, numPassed = 0;
    
    TEST("base64 class");
    FAILSTOP();
    FString data = "This is some data which we will be encoding in base64.";
    FString data64, decoded;
    Base64::Encode(data, data.size(), data64);
    cout << "Base64 encoded data is: " << data64 << endl;
    Base64::Decode(data64, decoded);
    cout << "Decoded : " << decoded << "(length=" << decoded.length() << ")" << endl;
    cout << "Original: " << data << "(length=" << data.length() << ")" << endl;
    if (data.compare(decoded)) FAIL("decoded data does not match original");
    ENDTEST();

    TEST("CBase64::Encode()   random binary data file 'random1'");
    FAILSTOP();
    FString randomData;
    FString randomDataEncoded;
    FString encoded;
    FString::LoadFile("random1", 0x100000, randomData);
    FString::LoadFile("random1encoded", 0x100000, randomDataEncoded);
    Base64::Encode(randomData, randomData.length(), encoded);
    if (randomDataEncoded.compare(encoded))
    {
        cout << "  Encoded: " << encoded << endl;
        cout << " Expected: " << randomDataEncoded << endl;
        FAIL("encoded data does not match expected");
    }
    ENDTEST();

    TEST("GUID generation");
    FString guid = GUID::GenerateGUID();
    cout << "GUID: " << guid << endl;
    ENDTEST();

    FString pubkey;
    FString privkey;

    FString plaintext;

    TEST("load keys");
    FAILSTOP();
    FString::LoadFile("pubkey.pem", 0x100000, pubkey);    
    FString::LoadFile("privkey.pem", 0x100000, privkey);    
    ENDTEST();

    TEST("base64 binary data");
    FAILSTOP();
    KeyBuffer publicKeyBuffer(pubkey);
    PublicKey publicKey(publicKeyBuffer, NULL);
    RSAString rsaStr("This is the plaintext.", publicKey);
    FString decoded, decoded2, tmp;
    cout << "   encoded: " << rsaStr << endl;
    Base64::Decode(rsaStr, decoded);
    Base64::Encode(decoded, decoded.length(), tmp);
    cout << "re-encoded: " << tmp << endl;
    Base64::Decode(tmp, decoded2);
    if (decoded.compare(decoded2)) FAIL("binary strings do not match");
    ENDTEST();

    TEST("encrypt string");
    FAILSTOP();
    KeyBuffer publicKeyBuffer(pubkey);
    PublicKey publicKey(publicKeyBuffer, NULL);

    KeyBuffer privateKeyBuffer(privkey);
    PrivateKey privateKey(privateKeyBuffer, "forte");
    FString plain("This is the plaintext.");
    RSAString rsaStr(plain, publicKey);
    cout << "Ciphertext: " << rsaStr << endl;
    rsaStr.GetPlainText(plaintext, privateKey);
    cout << "Plaintext: " << plaintext << endl;
    cout << "Plaintext length is " << plaintext.length() << endl;
    if (plaintext.compare(plain))
    {
        FAIL("plaintext does not match");
        cout << plain << " (" << plain.length() << ")" << endl;
        cout << plaintext << " (" << plaintext.length() << ")" << endl;
    }
    ENDTEST();

    TEST("account data envelope");
    FAILSTOP();
    KeyBuffer publicKeyBuffer(pubkey);
    KeyBuffer privateKeyBuffer(privkey);

    SecureEnvelopeEncoder encoder(publicKeyBuffer);
    SecureEnvelopeDecoder decoder(privateKeyBuffer, "forte");

    FString accountNum = "4111222233334444";
    FString obscuredNum = "XXXXXXXXXXXX4444";
    FString out, decoded, obscured;
    int mismatches = 0;
    int total = 1000;
    cout << "encoding credit card# " << accountNum << " over " << total << " iterations:" << endl;
    for (int i = 0; i < total; i++)
    {
        cout << "."; flush(cout);
        encoder.Encode(accountNum, 4, out);
//        cout << "encoded as: " << out << endl;
        decoder.Decode(out, decoded);
//        cout << "decoded as: " << decoded << endl;
        if (decoded.Compare(accountNum))
        {
            mismatches++;
            cout << "reattempting decode 10 times" << endl;
            int secondfailures = 0;
            for (int j = 0; j < 10; ++j)
            {
                decoder.Decode(out, decoded);
                if (decoded.Compare(accountNum)) secondfailures++;
                cout << "decoded as: " << decoded << endl;
            }
            cout << "secondary failures: " << secondfailures << endl;
        }
        SecureEnvelopeDecoder::Obscured(out, obscured);
//        cout << "obscured as: " << obscured << endl;
        if (obscured.Compare(obscuredNum))
        {
            FAIL("obscured account number does not match expected");
        }
    }
    cout << endl;
    cout << "Obscured as: " << obscured << endl;
    cout << " Decoded as: " << decoded << endl;
    if (mismatches > 0)
    {
        FAIL(FString(FStringFC(), "account number failed to decode %d times out of %d",
                     mismatches, total));
    }
    ENDTEST();

    TEST("Explode");
    std::vector<FString> components;
    FString t = "A,B,C,D,E,AB,AC,AD,AE,10.1.1.1,10.2.2.2 ,10.3.3.3, 10.4.4.4";
    t.Explode(",", components);
    std::vector<FString>::iterator i;
    cout << t << endl;
    cout << "Components: " << endl;
    for (i = components.begin(); i != components.end();++i)
    {
        cout << *i << endl;
    }
    if (components.size() != 13) FAIL("should have found 13 components");
    
    t = "A,B,C,D,";
    t.Explode(",", components);
    cout << t << endl;
    cout << endl << "Components: " << endl;
    for (i = components.begin(); i != components.end();++i)
    {
        cout << *i << endl;
    }
    if (components.size() != 5) FAIL("should have found 5 components");

    t = "A, B,C, D";
    t.Explode(", ", components);
    cout << t << endl;
    cout << endl << "Components: " << endl;
    for (i = components.begin(); i != components.end();++i)
    {
        cout << *i << endl;
    }
    if (components.size() != 3) FAIL("should have found 3 components");

    t = "A, B,C, D,";
    t.Explode(", ", components);
    cout << t << endl;
    cout << endl << "Components: " << endl;
    for (i = components.begin(); i != components.end();++i)
    {
        cout << *i << endl;
    }
    if (components.size() != 3) FAIL("should have found 3 components");

    t = "ABCDECDBCGBCDFBCD BCDBCDABCDBC";
    t.Explode("BCD", components);
    cout << t << endl;
    cout << endl << "Components: " << endl;
    for (i = components.begin(); i != components.end();++i)
    {
        cout << *i << endl;
    }
    if (components.size() != 7) FAIL("should have found 7 components");
    ENDTEST();

    std::vector<FString> lines;
    FString t = "Line1of2\r";
    TEST("LineSplit");
    if (t.LineSplit(lines) != 2) FAIL("should have 2 lines");
    foreach(FString &line, lines) cout << "Line: " << line << endl;
    ENDTEST();
    t = "Line1of2\rLine2of2";
    TEST("LineSplit");
    if (t.LineSplit(lines) != 2) FAIL("should have 2 lines");
    foreach(FString &line, lines) cout << "Line: " << line << endl;
    ENDTEST();
    t = "Line1of2\r\n";
    TEST("LineSplit");
    if (t.LineSplit(lines) != 2) FAIL("should have 2 lines");
    foreach(FString &line, lines) cout << "Line: " << line << endl;
    ENDTEST();
    t = "Line1of1";
    TEST("LineSplit");
    if (t.LineSplit(lines) != 1) FAIL("should have 1 lines");
    foreach(FString &line, lines) cout << "Line: " << line << endl;
    ENDTEST();
    t = "\rLine2of2";
    TEST("LineSplit");
    if (t.LineSplit(lines) != 2) FAIL("should have 2 lines");
    foreach(FString &line, lines) cout << "Line: " << line << endl;
    ENDTEST();
    t = "\nLine2of2";
    TEST("LineSplit");
    if (t.LineSplit(lines) != 2) FAIL("should have 2 lines");
    foreach(FString &line, lines) cout << "Line: " << line << endl;
    ENDTEST();
    t = "\nLine2of4\nLine3of4\n";
    TEST("LineSplit");
    if (t.LineSplit(lines) != 4) FAIL("should have 4 lines");
    foreach(FString &line, lines) cout << "Line: " << line << endl;
    ENDTEST();
    t = "\r\n\r\nLine3of3";
    TEST("LineSplit");
    if (t.LineSplit(lines) != 3) FAIL("should have 3 lines");
    foreach(FString &line, lines) cout << "Line: " << line << endl;
    ENDTEST();
    
    TEST("trim");
    FString t = " value\r\n";
    if (t.Trim().Compare("value"))
        FAIL("failed value trim");
    ENDTEST();

    TEST("trim 2");
    FString t = "Success\r\n";
    t.Trim();
    cout << "trimmed: \"" << t << "\"" << endl;
    if (t.Compare("Success"))
        FAIL("trim failed");
    ENDTEST();
    
    TEST("Explode/trim 1");
    FString t = "Header: value ";
    std::vector<FString> args;
    t.Explode(":", args, true);
    if (args.size() != 2)
    {
        FAIL("wrong number of Exploded components");
    }
    else if (args[0] != "Header")
    {
        FAIL("failed header parse");
    }
    else if (args[1] != "value")
    {
        FAIL("failed value parse");
    }
    ENDTEST();

    TEST("explode/trim 2");
    FString t = "Header: value\r\n";
    std::vector<FString> args;
    t.Explode(":", args, true);
    if (args.size() != 2)
    {
        FAIL("wrong number of Exploded components");
    }
    else if (args[0] != "Header")
    {
        FAIL("failed header parse");
    }
    else if (args[1] != "value")
    {
        FAIL("failed value parse");
    }
    ENDTEST();


    TEST("implode");
    std::vector<FString> components;
    components.push_back("1");
    components.push_back("2");
    components.push_back("3");
    components.push_back("4");
    components.push_back("5");
    components.push_back("6");
    components.push_back("7");
    components.push_back("8");
    components.push_back("9");
    components.push_back("10");
    FString result;
    cout << "imploded list of 1-10:" << endl << result.Implode(", ", components) << endl;
    if (result.Implode(", ", components).compare("1, 2, 3, 4, 5, 6, 7, 8, 9, 10"))
        FAIL("output does not match expected");
    ENDTEST();

    TEST("numeric");
    FString a = "0";
    if (!a.IsNumeric())
    {
        cout << "checking " << a << endl;
        FAIL("is numeric 1");
    }
    a = "-1";
    if (!a.IsNumeric())
        FAIL("is numeric 2");
    a = "-0.02";
    if (!a.IsNumeric())
        FAIL("is numeric 3");
    a = "1.45";
    if (!a.IsNumeric())
        FAIL("is numeric 4");
    a = "1454234823908490238490234234234235234523";
    if (!a.IsNumeric())
        FAIL("is numeric 5");
    a = "";
    if (a.IsNumeric())
        FAIL("is numeric 6");

    a = "0";
    if (!a.IsUnsignedNumeric())
        FAIL("is unsigned numeric 1");
    a = "-1";
    if (a.IsUnsignedNumeric())
        FAIL("is unsigned numeric 2");
    a = "-0.02";
    if (a.IsUnsignedNumeric())
        FAIL("is unsigned numeric 3");
    a = "1.45";
    if (!a.IsUnsignedNumeric())
        FAIL("is unsigned numeric 4");
    a = "1454234823908490238490234234234235234523";
    if (!a.IsUnsignedNumeric())
        FAIL("is unsigned numeric 5");
    a = "-1454234823908490238490234234234235234523";
    if (a.IsUnsignedNumeric())
        FAIL("is unsigned numeric 5");

    
    ENDTEST();

    SUMMARY();
    return 0;
}
#endif
