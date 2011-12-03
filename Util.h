#ifndef __Forte_Util_h_
#define __Forte_Util_h_

#include "FString.h"

////timeval conversion functions //////////////////////////////////////////////
inline struct timeval operator - (const struct timeval &a, const struct timeval &b)
{
    struct timeval result;
    result.tv_sec = a.tv_sec - b.tv_sec;
    if (a.tv_usec > b.tv_usec)
        result.tv_usec = a.tv_usec - b.tv_usec;
    else
    {
        --result.tv_sec;
        result.tv_usec = 1000000 + a.tv_usec - b.tv_usec;
    }
    return result;
}
inline struct timeval operator + (const struct timeval &a, const struct timeval &b)
{
    struct timeval result;
    result.tv_sec = a.tv_sec + b.tv_sec;
    result.tv_usec = a.tv_usec + b.tv_usec;
    if (result.tv_usec >= 1000000)
    {
        result.tv_usec -= 1000000;
        result.tv_sec += 1;
    }
    return result;
}
inline struct timeval operator * (const struct timeval &a, const int &b)
{
    struct timeval result;
    result.tv_sec = a.tv_sec * b;
    result.tv_usec = a.tv_usec * b;
    result.tv_sec += result.tv_usec / 1000000;
    result.tv_usec %= 1000000;
    return result;
}
inline bool operator < (const struct timeval &a, const struct timeval &b)
{
    if (a.tv_sec < b.tv_sec) return true;
    else if (a.tv_sec == b.tv_sec && a.tv_usec < b.tv_usec) return true;
    else return false;
}

////timespec conversion functions///////////////////////////////////////////////
inline struct timespec operator - (const struct timespec &a, 
                                   const struct timespec &b)
{
    struct timespec result;
    result.tv_sec = a.tv_sec - b.tv_sec;
    if (a.tv_nsec >= b.tv_nsec)
        result.tv_nsec = a.tv_nsec - b.tv_nsec;
    else
    {
        --result.tv_sec;
        result.tv_nsec = 1000000000 + a.tv_nsec - b.tv_nsec;
    }
    return result;
}
inline struct timespec operator + (const struct timespec &a, 
                                   const struct timespec &b)
{
    struct timespec result;
    result.tv_sec = a.tv_sec + b.tv_sec;
    result.tv_nsec = a.tv_nsec + b.tv_nsec;
    if (result.tv_nsec >= 1000000000)
    {
        result.tv_nsec -= 1000000000;
        result.tv_sec += 1;
    }
    return result;
}
inline struct timespec operator * (const struct timespec &a, const int &b)
{
    // TODO:  this is buggy, potential int overflow in the nsec field
    struct timespec result;
    result.tv_sec = a.tv_sec * b;
    result.tv_nsec = a.tv_nsec * b;
    result.tv_sec += result.tv_nsec / 1000000000;
    result.tv_nsec %= 1000000000;
    return result;
}
inline bool operator < (const struct timespec &a, const struct timespec &b)
{
    if (a.tv_sec < b.tv_sec) return true;
    else if (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec) return true;
    else return false;
}
inline bool operator <= (const struct timespec &a, const struct timespec &b)
{
    return !(b < a);
}

#endif
