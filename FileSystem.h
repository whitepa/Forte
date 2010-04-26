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


namespace Forte
{
// typed exceptions
    EXCEPTION_SUBCLASS(Exception, EFileSystem);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemCopy);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemLink);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemMkdir);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemReadlink);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemRename);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemResolveSymlink);
//not used yet, might be desirable
//EXCEPTION_SUBCLASS(EFileSystemResolveSymlink, ForteFileSystemResolveSymlinkBroken);
//EXCEPTION_SUBCLASS(EFileSystemResolveSymlink, ForteFileSystemResolveSymlinkLoop);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemSymLink);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemTouch);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemUnlink);


    class FileSystem : public Object
    {
    public:
        FileSystem();
        virtual ~FileSystem();

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
        static void DeleteSingleton();

        // types
        typedef std::map<ino_t, std::string> InodeMap;
        typedef void (*progress_callback_t)(uint64_t, void*);

        // interface
        virtual FString GetCWD();
        virtual void Touch(const FString& file);
        virtual bool FileExists(const FString& filename);
        virtual int Stat(const FString& path, struct stat *st);
        virtual bool IsDir(const FString& path);
        virtual int LStat(const FString& path, struct stat *st);
        virtual int StatAt(int dir_fd, const FString& path, struct stat *st);
        virtual int LStatAt(int dir_fd, const FString& path, struct stat *st);

        virtual int FStatAt(
            int dir_fd, 
            const FString& path, 
            struct stat *st, 
            int flags = 0);

        /**
         * \ref unlinkHelper
         **/
        virtual void Unlink(const FString& path, bool unlink_children = false,
                            progress_callback_t progress_callback = NULL,
                            void *callback_data = NULL);

        virtual void UnlinkAt(int dir_fd, const FString& path);
        virtual void Rename(const FString& from, const FString& to);
        virtual void RenameAt(int dir_from_fd, const FString& from,
                              int dir_to_fd, const FString& to);
        virtual void MakeDir(const FString& path, mode_t mode = 0777, bool make_parents = false);
        virtual void MakeDirAt(int dir_fd, const FString& path, mode_t mode = 0777);

        inline void MakeFullPath(const FString& path, mode_t mode = 0777) 
        { 
            MakeDir(path, mode, true); 
        }

        virtual void Link(const FString& from, const FString& to);
        virtual void LinkAt(int dir_from_fd, const FString& from, 
                            int dir_to_fd, const FString& to);

        virtual void SymLink(const FString& from, const FString& to);
        virtual void SymLinkAt(const FString& from, int dir_to_fd, const FString& to);
        virtual FString ReadLink(const FString& path);
        virtual FString ResolveSymLink(const FString& path);
        virtual FString FullyResolveSymLink(const FString& path);

        virtual FString MakeRelativePath(const FString& base, 
                                         const FString& path);
        virtual FString ResolveRelativePath(const FString& base, 
                                            const FString& path);

        virtual void FileCopy(const FString& from, 
                               const FString& to, 
                               mode_t mode = 0777);

        virtual FString FileGetContents(const FString& filename);
        virtual void FilePutContents(const FString& filename, 
                                     const FString& data);

        /// deep_copy copies a directory tree from 'source' to 'dest'.
        virtual void DeepCopy(const FString& source, const FString& dest, 
                              progress_callback_t progress_callback = NULL,
                              void *callback_data = NULL);


        virtual void Copy(const FString& from_path, const FString& to_path,
                          progress_callback_t progress_callback = NULL,
                          void *callback_data = NULL);


        // error messages
        virtual FString StrError(int err /*errno*/) const;

    protected:
        // helpers

        // ctor/dtor/singleton
        FileSystem();
        virtual ~FileSystem();
        static FileSystem *sSingleton;
        static Mutex sMutex;
    };

        virtual void deepCopyHelper(
            const FString& source, 
            const FString& dest, 
            const FString& dir, 
            InodeMap &inode_map, 
            uint64_t &size_copied,
            progress_callback_t progress_callback = NULL,
            void *callback_data = NULL);


        /**
         * unlink just one path (no recursion)
         **/
        virtual void unlinkHelper(const FString& path);  
    };
};
#endif
