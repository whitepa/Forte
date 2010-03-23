#ifndef __ansi_test_h
#define __ansi_test_h

#include <iostream>

using namespace std;

// macros
#ifdef TEST
#  undef TEST
#endif

#define TEST(X, Y, DESC)                                        \
    try                                                         \
    {                                                           \
        cout << DESC;  cout.flush();                            \
        if ((X) != (Y))                                         \
        {                                                       \
            all_pass = false;                                   \
            cout << ANSI_FAIL << endl;                          \
            cout << "\t" << (X) << " != " << (Y) << endl;       \
        }                                                       \
        else cout << ANSI_PASS << endl;                         \
    }                                                           \
    catch (Exception& e)                                       \
    {                                                           \
        all_pass = false;                                       \
        cout << ANSI_FAIL << endl;                              \
        cout << "\tException: " << e.getDescription() << endl; \
        e.pretty_trace_log(HLOG_ERR);                           \
    }                                                           \
    catch (std::exception e)                                    \
    {                                                           \
        all_pass = false;                                       \
        cout << ANSI_FAIL << endl;                              \
        cout << "\tstd::exception: " << e.what() << endl;       \
    }                                                           \
    catch (...)                                                 \
    {                                                           \
        all_pass = false;                                       \
        cout << ANSI_FAIL << endl;                              \
        cout << "\tUnknown exception" << endl;                  \
    }

#define TEST_NO_EXP(X, DESC)                                    \
    try                                                         \
    {                                                           \
        cout << DESC;  cout.flush();                            \
        X;                                                      \
        cout << ANSI_PASS << endl;                              \
    }                                                           \
    catch (Exception& e)                                       \
    {                                                           \
        all_pass = false;                                       \
        cout << ANSI_FAIL << endl;                              \
        cout << "\tException: " << e.getDescription() << endl; \
        e.pretty_trace_log(HLOG_ERR);                           \
    }                                                           \
    catch (std::exception e)                                    \
    {                                                           \
        all_pass = false;                                       \
        cout << ANSI_FAIL << endl;                              \
        cout << "\tstd::exception: " << e.what() << endl;       \
    }                                                           \
    catch (...)                                                 \
    {                                                           \
        all_pass = false;                                       \
        cout << ANSI_FAIL << endl;                              \
        cout << "\tUnknown exception" << endl;                  \
    }

#define TEST_EXP(X, Y, DESC)                                        \
    try                                                             \
    {                                                               \
        cout << DESC;  cout.flush();                                \
        X;                                                          \
        all_pass = false;                                           \
        cout << ANSI_FAIL << endl;                                  \
        cout << "\tNo exception thrown" << endl;                    \
    }                                                               \
    catch (Exception& e)                                           \
    {                                                               \
        if (e.getDescription() == (Y))                              \
        {                                                           \
            cout << ANSI_PASS << endl;                              \
        }                                                           \
        else                                                        \
        {                                                           \
            all_pass = false;                                       \
            cout << ANSI_FAIL << endl;                              \
            cout << "\tException: " << e.getDescription() << endl; \
            e.pretty_trace_log(HLOG_ERR);                           \
        }                                                           \
    }                                                               \
    catch (std::exception e)                                        \
    {                                                               \
        if (FString(e.what()) == (Y))                               \
        {                                                           \
            cout << ANSI_PASS << endl;                              \
        }                                                           \
        else                                                        \
        {                                                           \
            all_pass = false;                                       \
            cout << ANSI_FAIL << endl;                              \
            cout << "\tstd::exception: " << e.what() << endl;       \
        }                                                           \
    }                                                               \
    catch (...)                                                     \
    {                                                               \
        all_pass = false;                                           \
        cout << ANSI_FAIL << endl;                                  \
        cout << "\tUnknown exception" << endl;                      \
    }

// constants
static const char *ANSI_PASS = "\033[1;32m\033[70GPASS\033[0;39m";
static const char *ANSI_FAIL = "\033[1;31m\033[70GFAIL\033[0;39m";

// globals
static bool all_pass = true;

#endif
