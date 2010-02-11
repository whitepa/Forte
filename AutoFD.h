#ifndef __forte_auto_fd_h
#define __forte_auto_fd_h

#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>


namespace Forte
{
    class AutoFD
    {
    public:
        static const int NONE = -1;
        AutoFD(int fd = NONE) : m_dir(NULL), m_fd(fd) { }
        AutoFD(DIR *dir) : m_dir(dir) { if (m_dir != NULL) m_fd = dirfd(dir); }
        ~AutoFD() { close(); }
        inline void close() {
            if (m_dir != NULL) { closedir(m_dir); m_dir = NULL; m_fd = NONE; }
            if (m_fd != NONE) { ::close(m_fd); m_fd = NONE; }
        }
        inline void release() { m_dir = NULL; m_fd = NONE; }
        inline int fd() const { return m_fd; }
        inline DIR* dir() { return m_dir; }
        inline const DIR* dir() const { return m_dir; }
        inline operator int() const { return m_fd; }
        inline operator DIR*() { return m_dir; }
        inline operator const DIR*() const { return m_dir; }
        inline AutoFD& operator =(int fd) { close(); m_fd = fd; return *this; }
        inline AutoFD& operator =(DIR *dir) {
            close();
            if ((m_dir = dir) != NULL) m_fd = dirfd(m_dir);
            return *this;
        }
    private:
        DIR *m_dir;
        int m_fd;
    };
};

#endif
