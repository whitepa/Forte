#ifndef __forte_FileSystem_h
#define __forte_FileSystem_h

#include "Context.h"
#include "Types.h"
#include "LogManager.h"
#include "AutoMutex.h"
#include "AutoFD.h"
#include "ProcRunner.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <boost/function.hpp>

namespace Forte
{
    // typed exceptions
    EXCEPTION_SUBCLASS(Exception, EFileSystem);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemCopy);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemAppend);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemMakeDir);
    EXCEPTION_SUBCLASS(EFileSystem, EFileSystemResolveSymLink);

    class FileSystem : public Object
    {
    public:
        FileSystem() {}
        virtual ~FileSystem() {}

        // types
        typedef boost::function<void (uint64_t)> ProgressCallback;

        // interface
        virtual FString Basename(const FString& filename,
                                 const FString& suffix = "") = 0;
        virtual FString Dirname(const FString& filename) = 0;
        virtual FString GetCWD() = 0;
        virtual FString GetPathToCurrentProcess() = 0;
        virtual unsigned int Glob(std::vector<FString> &resultVec,
                                  const FString &pattern,
                                  const int globFlags = 0) const = 0;
        virtual void Touch(const FString& file) = 0;
        virtual bool FileExists(const FString& filename) const = 0;
        virtual void StatFS(const FString& path, struct statfs *st) = 0;
        virtual int Stat(const FString& path, struct stat *st) = 0;
        virtual bool IsDir(const FString& path) const = 0;
        virtual void GetChildren(const FString& path,
                                 std::vector<Forte::FString> &children,
                                 bool recurse = false,
                                 bool includePathInChildNames = true,
                                 bool includePathNames = false) const = 0;
        virtual uint64_t CountChildren(
            const FString& path, bool recurse) const = 0;
        virtual int LStat(
            const FString& path, struct stat *st) = 0;
        virtual int StatAt(
            int dir_fd, const FString& path, struct stat *st) = 0;
        virtual int LStatAt(
            int dir_fd, const FString& path, struct stat *st) = 0;
        virtual int FStatAt(
            int dir_fd, const FString& path, struct stat *st, int flags = 0) = 0;

        virtual void Unlink(
            const FString& path, bool unlink_children = false,
           const ProgressCallback &progressCallback = ProgressCallback()) = 0;

        virtual void UnlinkAt(int dir_fd, const FString& path) = 0;
        virtual void Rename(const FString& from, const FString& to) = 0;
        virtual void RenameAt(int dir_from_fd, const FString& from,
                              int dir_to_fd, const FString& to) = 0;

        virtual void ChangeOwner(const FString& path, uid_t uid, gid_t gid) = 0;

        virtual void MakeDir(const FString& path, mode_t mode = 0777,
                             bool make_parents = false) = 0;
        virtual void MakeDirAt(int dir_fd,
                               const FString& path,
                               mode_t mode = 0777) = 0;
        virtual void MakeFullPath(const FString& path, mode_t mode = 0777) = 0;
        virtual FString MakeRelativePath(const FString& base,
                                         const FString& path) = 0;
        virtual FString ResolveRelativePath(const FString& base,
                                            const FString& path) = 0;

        virtual int ScanDir(const FString& path,
                            std::vector<FString> &namelist) = 0;

        virtual void Link(const FString& from, const FString& to) = 0;
        virtual void LinkAt(int dir_from_fd, const FString& from,
                            int dir_to_fd, const FString& to) = 0;
        virtual void SymLink(const FString& from, const FString& to) = 0;
        virtual void SymLinkAt(
            const FString& from, int dir_to_fd, const FString& to) = 0;
        virtual FString ReadLink(const FString& path) = 0;
        virtual FString ResolveSymLink(const FString& path) = 0;
        virtual FString FullyResolveSymLink(const FString& path) = 0;

        virtual FString FileGetContents(const FString& filename) const = 0;
        virtual void FilePutContents(const FString& filename,
                                     const FString& data,
                                     bool append=false,
                                     bool throwOnError=false) = 0;

        virtual void FilePutContentsWithPerms(
            const FString& filename,
            const FString& data,
            int flags,
            int mode) = 0;

        virtual void FileOpen(AutoFD &autoFd, const FString &path, int flags,
                              int mode) = 0;

        virtual void FileAppend(const FString& from, const FString& to) = 0;

        virtual void FileCopy(const FString& from,
                              const FString& to,
                              mode_t mode = 0777) = 0;
        virtual void DeepCopy(
            const FString& source, const FString& dest,
            const ProgressCallback &progressCallback = ProgressCallback()) = 0;


        virtual void Copy(
            const FString& from_path, const FString& to_path,
            const ProgressCallback &progressCallback = ProgressCallback()) = 0;

        virtual FString StrError(int err /*errno*/) const = 0;

        virtual FString MakeTemporaryFile(const FString& nameTemplate) const = 0;

        virtual void Truncate(const FString& path, off_t size) const = 0;
    };

    typedef boost::shared_ptr<FileSystem> FileSystemPtr;
};
#endif
