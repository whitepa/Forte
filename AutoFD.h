#ifndef __forte_auto_fd_h
#define __forte_auto_fd_h

#include "Object.h"
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>


namespace Forte
{
    class AutoFD : public Object
    {
    public:
        static const int NONE = -1;
        AutoFD(int fd = NONE) : mDir(NULL), mFD(fd) { }
        AutoFD(DIR *dir) : mDir(dir) { if (mDir != NULL) mFD = dirfd(dir); }
        ~AutoFD() { Close(); }
        inline void Close() {
            if (mDir != NULL) { closedir(mDir); mDir = NULL; mFD = NONE; }
            if (mFD != NONE) { ::close(mFD); mFD = NONE; }
        }
        inline void Release() { mDir = NULL; mFD = NONE; }
        inline int Fd() const { return mFD; }
        inline DIR* dir() { return mDir; }
        inline const DIR* dir() const { return mDir; }
        inline operator int() const { return mFD; }
        inline operator DIR*() { return mDir; }
        inline operator const DIR*() const { return mDir; }
        inline AutoFD& operator =(int fd) { Close(); mFD = fd; return *this; }
        inline AutoFD& operator =(DIR *dir) {
            Close();
            if ((mDir = dir) != NULL) mFD = dirfd(mDir);
            return *this;
        }
    private:
        DIR *mDir;
        int mFD;
    };
};

#endif
