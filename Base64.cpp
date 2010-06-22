#include "Base64.h"

using namespace Forte;

static char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline int pos(char c)
{
    if ('A' <= c && c <= 'Z') return c - 'A';
    if ('a' <= c && c <= 'z') return c - 'a' + 26;
    if ('0' <= c && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

int Base64::Encode(const char *data, int size, FString &out)
{
    char *s, *p;
    int i;
    int c;
    const unsigned char *q;
    
    p = s = (char*)malloc(size*4/3+4);
    if (p == NULL)
        return -1;
    q = (const unsigned char*)data;
    i=0;
    for(i = 0; i < size;){
        c=q[i++];
        c*=256;
        if(i < size)
            c+=q[i];
        i++;
        c*=256;
        if(i < size)
            c+=q[i];
        i++;
        p[0]=chars[(c&0x00fc0000) >> 18];
        p[1]=chars[(c&0x0003f000) >> 12];
        p[2]=chars[(c&0x00000fc0) >> 6];
        p[3]=chars[(c&0x0000003f) >> 0];
        if(i > size)
            p[3]='=';
        if(i > size+1)
            p[2]='=';
        p+=4;
    }
    *p=0;
    out.assign(s);
    free(s);
    return out.length();
}

int Base64::Decode(const char *in, FString &out)
{
    out.clear();
    out.reserve(strlen(in) * 3 / 4 + 3);
    const char *p;
    int done = 0;
    int c;
    int x;
    for (p = in; *p && !done; p+=4)
    {
        if ((x = pos(p[0])) != -1)
            c = x;
        else
        {
            done = 3;
            break;
        }
        c*=64;
        
        if ((x = pos(p[1]))==-1) throw EBase64("invalid base64 data");
        c+=x;
        c*=64;

        if (p[2] == '=') done++;
        else
        {
            if ((x = pos(p[2]))==-1) throw EBase64("invalid base64 data");
            c+=x;
        }
        c*=64;
        
        if (p[3] == '=') done++;
        else
        {
            if (done) throw EBase64("invalid base64 data");
            if ((x=pos(p[3]))==-1) throw EBase64("invalid base64 data");
            c+=x;
        }
        if (done < 3)
            out.append(1,(c&0xff0000)>>16);
        if (done < 2)
            out.append(1,(c&0xff00)>>8);
        if (done < 1)
            out.append(1,c&0xff);
    }
    return out.length();
}
