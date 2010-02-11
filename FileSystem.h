#ifndef __forte_FileSystem_h
#define __forte_FileSystem_h

#include "Types.h"
#include "LogManager.h"
#include "AutoMutex.h"
#include "AutoFD.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

// typed exceptions
EXCEPTION_SUBCLASS(CForteException, CForteFileSystemException);
EXCEPTION_SUBCLASS(CForteFileSystemException, CForteFileSystemCopyException);
EXCEPTION_SUBCLASS(CForteFileSystemException, CForteFileSystemLinkException);
EXCEPTION_SUBCLASS(CForteFileSystemException, CForteFileSystemMkdirException);
EXCEPTION_SUBCLASS(CForteFileSystemException, CForteFileSystemReadlinkException);
EXCEPTION_SUBCLASS(CForteFileSystemException, CForteFileSystemRenameException);
EXCEPTION_SUBCLASS(CForteFileSystemException, CForteFileSystemResolveSymlinkException);
//not used yet, might be desirable
//EXCEPTION_SUBCLASS(CForteFileSystemResolveSymlinkException, CForteFileSystemResolveSymlinkBroken);
//EXCEPTION_SUBCLASS(CForteFileSystemResolveSymlinkException, CForteFileSystemResolveSymlinkLoop);
EXCEPTION_SUBCLASS(CForteFileSystemException, CForteFileSystemSymLinkException);
EXCEPTION_SUBCLASS(CForteFileSystemException, CForteFileSystemTouchException);
EXCEPTION_SUBCLASS(CForteFileSystemException, CForteFileSystemUnlinkException);


class FileSystem
{
public:
    // helper classes
    class AdvisoryLock
    {
    public:
        AdvisoryLock(int fd, off64_t start, off64_t len, short whence = SEEK_SET);
        
        /// getLock returns a lock description equivalent to the lock
        /// currently blocking us.
        AdvisoryLock getLock(bool exclusive = false);

        /// sharedLock will return true on success, false if the lock failed
        ///
        bool sharedLock(bool wait = true);
        
        /// exclusiveLock will return true on success, false if the lock failed
        ///
        bool exclusiveLock(bool wait = true);

        /// unlock will remove the current lock
        ///
        void unlock(void);

    protected:
        struct flock m_lock;
        int m_fd;
    };

    class AdvisoryAutoUnlock
    {
    public:
        AdvisoryAutoUnlock(int fd, off64_t start, off64_t len, bool exclusive, 
                           short whence = SEEK_SET) :
            m_advisoryLock(fd, start, len, whence)
            {
                if (exclusive)
                    m_advisoryLock.exclusiveLock();
                else
                    m_advisoryLock.sharedLock();
            }
        virtual ~AdvisoryAutoUnlock() { m_advisoryLock.unlock(); }
    protected:
        AdvisoryLock m_advisoryLock;
    };

    // singleton
    static FileSystem* get();
    static FileSystem& getRef();

    // types
    typedef std::map<ino_t, std::string> InodeMap;
    typedef void (*progress_callback_t)(uint64_t, void*);

    // interface
    virtual FString getcwd();
    virtual void touch(const FString& file);
    virtual bool file_exists(const FString& filename);
    virtual int stat(const FString& path, struct stat *st);
    virtual bool is_dir(const FString& path);
    virtual int lstat(const FString& path, struct stat *st);
    virtual int statat(int dir_fd, const FString& path, struct stat *st);
    virtual int lstatat(int dir_fd, const FString& path, struct stat *st);
    virtual int fstatat(int dir_fd, const FString& path, struct stat *st, int flags = 0);
    virtual void unlink(const FString& path, bool unlink_children = false,
                        progress_callback_t progress_callback = NULL,
                        void *callback_data = NULL); // see unlink_helper() below
    virtual void unlinkat(int dir_fd, const FString& path);
    virtual void rename(const FString& from, const FString& to);
    virtual void renameat(int dir_from_fd, const FString& from,
                            int dir_to_fd, const FString& to);
    virtual void mkdir(const FString& path, mode_t mode = 0777, bool make_parents = false);
    virtual void mkdirat(int dir_fd, const FString& path, mode_t mode = 0777);
    inline void mkdir_all(const FString& path, mode_t mode = 0777) { mkdir(path, mode, true); }
    virtual void link(const FString& from, const FString& to);
    virtual void linkat(int dir_from_fd, const FString& from, int dir_to_fd, const FString& to);
    virtual void symlink(const FString& from, const FString& to);
    virtual void symlinkat(const FString& from, int dir_to_fd, const FString& to);
    virtual FString readlink(const FString& path);
    virtual FString resolve_symlink(const FString& path);
    virtual FString fully_resolve_symlink(const FString& path);
    virtual FString make_rel_path(const FString& base, const FString& path);
    virtual FString resolve_rel_path(const FString& base, const FString& path);
    virtual void file_copy(const FString& from, const FString& to, mode_t mode = 0777);
    virtual FString file_get_contents(const FString& filename);
    virtual void file_put_contents(const FString& filename, const FString& data);

    /// deep_copy copies a directory tree from 'source' to 'dest'.
    virtual void deep_copy(const FString& source, const FString& dest, 
                           progress_callback_t progress_callback = NULL,
                           void *callback_data = NULL);
    virtual void deep_copy_helper(const FString& source, const FString& dest, 
                                  const FString& dir, 
                                  InodeMap &inode_map, uint64_t &size_copied,
                                  progress_callback_t progress_callback = NULL,
                                  void *callback_data = NULL);
    virtual void copy(const FString& from_path, const FString& to_path,
                      progress_callback_t progress_callback = NULL,
                      void *callback_data = NULL);
    virtual void copy_helper(const FString& from_path, const FString& to_path, 
                             const struct stat& st,
                             InodeMap &inode_map/*IN-OUT*/, uint64_t &size_copied/*IN-OUT*/,
                             progress_callback_t progress_callback = NULL,
                             void *callback_data = NULL);

    // error messages
    virtual FString strerror(int err /*errno*/) const;

protected:
    // helpers
    virtual void unlink_helper(const FString& path);  // unlink just one path (no recursion)

    // ctor/dtor/singleton
    FileSystem();
    virtual ~FileSystem();
    static FileSystem *s_singleton;
    static CMutex s_mutex;
};

// inline funcs for legacy code support
inline bool file_exists(const FString& filename)
{
    return FileSystem::get()->file_exists(filename);
}

inline FString file_get_contents(const FString& filename)
{
    return FileSystem::get()->file_get_contents(filename);
}

inline void file_put_contents(const FString& filename, const FString& data)
{
    FileSystem::get()->file_put_contents(filename, data);
}

#endif
