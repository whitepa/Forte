/* MD5.H - header file for MD5C.C
 */

#ifndef __forte_md5_h
#define __forte_md5_h

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
*/

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <cstdlib>
#include "FString.h"

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef uint32_t UINT4;

/* BYTE defines a one-byte word */
typedef unsigned char BYTE;

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
   If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
   returns an empty list.
*/
#define PROTO_LIST(list) list

/* MD5 context. */
typedef struct {
    UINT4 state[4];                                   /* state (ABCD) */
    UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

void MD5Init PROTO_LIST ((MD5_CTX *));
void MD5Update PROTO_LIST ((MD5_CTX *, unsigned char *, unsigned int));
void MD5Final PROTO_LIST ((unsigned char [16], MD5_CTX *));

class MD5
{
// data members
public:
    unsigned char md5[16];

protected:
    MD5_CTX m_ctx;

// ctor/dtor
public:
    MD5() { Init(); memset(md5, 0, 16); }
    MD5(unsigned char m[16]) { Init(); memcpy(md5, m, 16); }
    MD5(const MD5& other) { Init(); *this = other; }
    MD5(const FString& str) { Init(); *this = str; }

// operations
public:
    inline void Init() { MD5Init(&m_ctx); }
    inline void Update(const void *data, unsigned size) { MD5Update(&m_ctx, (unsigned char*)data, size); }
    inline void Final() { MD5Final(md5, &m_ctx); }
    MD5& operator=(const MD5& other)
    {
        memcpy(md5, other.md5, 16);
        memcpy(&m_ctx, &other.m_ctx, sizeof(m_ctx));
        return *this;
    }
    MD5& operator=(const FString& str) { return *this = AtoMD5(str); }
    operator FString() const { return MD5toA(*this); }
    static MD5 AtoMD5(const FString& str);
    static FString MD5toA(const MD5& md5);
    bool operator==(const MD5& other) const { return memcmp(md5, other.md5, 16) == 0; }
};

#endif
