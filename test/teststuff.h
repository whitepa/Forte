#ifndef _teststuff_h
#define _teststuff_h

#include "Forte.h"

#if !defined(FORTE_WITH_DATETIME)
#define TEST(name) {                                    \
        cout << "TEST \"" << name << "\"..." << endl;   \
        bool testfail = false, failstop = false;
#else
#define TEST(name) {                                    \
        cout << "TEST \"" << name << "\"..." << endl;   \
        struct timeval teststart;                            \
        gettimeofday(&teststart, NULL);                      \
        printf("timestamp=%s.%06u\n", FTime::to_str(teststart.tv_sec, "US/Pacific").c_str(), (unsigned int)teststart.tv_usec);
        bool testfail = false, failstop = false;
#endif

#define FAIL(why) { testfail = true; cout << "FAIL>   " << why << endl; }

#define ENDTEST() cout << ((!testfail) ? "[PASS]" : "[FAILURE]") << endl; cout << "*********" << endl; \
    if (failstop && testfail) { cout << "Stopping due to failure." << endl; exit(1);} \
    if (testfail) ++numFailed; else ++numPassed; }

#define CALL(call)                              \
    proxy.reset();                              \
    if (proxy.call != SOAP_OK)                  \
    {                                           \
        soap_print_fault(proxy.soap, stdout);   \
        testfail = true;                        \
    }

#define CALLFAIL(call)     proxy.reset();                               \
    if (proxy.call == SOAP_OK)                                          \
    {                                                                   \
        cout << "SOAP call returned SOAP_OK (should have failed)" << endl; \
        testfail = true;                                                \
    }

#define CALLDIALOG(call, dialogEx) \
    dialogEx = NULL;                                                    \
    if (proxy.call != SOAP_OK) {                                        \
        if (proxy.soap->fault)                                          \
        {                                                               \
            if (proxy.soap->version == 1 && proxy.soap->fault->detail)  \
            {                                                           \
                if (proxy.soap->fault->detail->__type == SOAP_TYPE_SoapDialogException) \
                {                                                       \
                    cout << "Received SoapDialogException" << endl;     \
                    soap_print_fault(proxy.soap, stdout);               \
                    dialogEx = (SoapDialogException*)proxy.soap->fault->detail->fault; \
                }                                                       \
                else                                                    \
                {                                                       \
                    cout << "fault detail is not a dialog" << endl;     \
                    soap_print_fault(proxy.soap,stdout);                \
                    testfail = true;                                    \
                }                                                       \
            }                                                           \
            else if (proxy.soap->version == 2 && proxy.soap->fault->SOAP_ENV__Detail) \
            {                                                           \
                if (proxy.soap->fault->SOAP_ENV__Detail->__type == SOAP_TYPE_SoapDialogException) \
                {                                                       \
                    cout << "Received SoapDialogException" << endl;     \
                    soap_print_fault(proxy.soap, stdout);               \
                    dialogEx = (SoapDialogException*)proxy.soap->fault->SOAP_ENV__Detail->fault; \
                }                                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                soap_print_fault(proxy.soap,stdout);                    \
                testfail = true;                                        \
            }                                                           \
        }                                                               \
    }

#define DIALOGPUSH(dr) \
    if (!proxy.soap->header)                          \
        proxy.soap->header = ::soap_new_SOAP_ENV__Header(proxy.soap, -1); \
    proxy.soap->header->responses.push_back(dr);

#define FAILSTOP() failstop = true;

#define SUMMARY()                               \
    cout << endl;                               \
    cout << " PASSED: " << numPassed << endl;   \
    cout << " FAILED: " << numFailed << endl;


#endif
