#ifndef __forte_FileSystemImpl_h
#define __forte_FileSystemImpl_h

#include "FileSystem.h"

namespace Forte
{
    class FileSystemImpl : public FileSystem
    {
    public:
        FileSystemImpl();
        virtual ~FileSystemImpl();

        // types
        typedef std::map<ino_t, std::string> InodeMap;

        // interface
        virtual FString Basename(const FString& filename,
                                 const FString& suffix = "");
        virtual FString Dirname(const FString& filename);
        virtual FString GetCWD();
        virtual FString GetPathToCurrentProcess();
        virtual unsigned int Glob(std::vector<FString> &resultVec,
                                  const FString &pattern,
                                  const int globFlags = 0) const;
        virtual void Touch(const FString& file);
        virtual bool FileExists(const FString& filename) const;

        virtual void StatFS(const FString& path, struct statfs *st);

        virtual int Stat(const FString& path, struct stat *st);
        virtual bool IsDir(const FString& path) const;
        virtual void GetChildren(const FString& path,
                                 std::vector<Forte::FString> &children,
                                 bool recurse = false,
                                 bool includePathInChildNames = true) const;
        uint64_t CountChildren(const FString& path, bool recurse) const;
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
                const ProgressCallback &progressCallback = ProgressCallback());

        virtual void UnlinkAt(int dir_fd, const FString& path);
        virtual void Rename(const FString& from, const FString& to);
        virtual void RenameAt(int dir_from_fd, const FString& from,
                              int dir_to_fd, const FString& to);
        virtual void ChangeOwner(const FString& path, uid_t uid, gid_t gid);
        virtual void MakeDir(const FString& path, mode_t mode = 0777, bool make_parents = false);
        virtual void MakeDirAt(int dir_fd, const FString& path, mode_t mode = 0777);
        virtual int ScanDir(const FString& path, std::vector<FString> &namelist);
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

        virtual FString FileGetContents(const FString& filename) const;

        virtual void FilePutContents(const FString& filename,
                                     const FString& data,
                                     bool append=false,
                                     bool throwOnError=false);

        virtual void FilePutContentsWithPerms(
            const FString& filename,
            const FString& data,
            int flags,
            int mode);


        virtual void FileOpen(AutoFD &autoFd, const FString &path, int flags,
                              int mode);

        virtual void FileAppend(const FString& from, const FString& to);

        /// deep_copy copies a directory tree from 'source' to 'dest'.
        virtual void DeepCopy(const FString& source, const FString& dest,
                const ProgressCallback &progressCallback = ProgressCallback());


        virtual void Copy(const FString& from_path, const FString& to_path,
                          const ProgressCallback &progressCallback = ProgressCallback());
        // error messages
        virtual FString StrError(int err /*errno*/) const;

        virtual FString MakeTemporaryFile(const FString& nameTemplate) const;
    protected:
        // helpers

        virtual void copyHelper(const FString& from_path,
                                const FString& to_path,
                                const struct stat& st,
                                InodeMap &inode_map/*IN-OUT*/,
                                uint64_t &size_copied/*IN-OUT*/,
                                const ProgressCallback &progressCallback = ProgressCallback());

        virtual void deepCopyHelper(
            const FString& source,
            const FString& dest,
            const FString& dir,
            InodeMap &inode_map,
            uint64_t &size_copied,
            const ProgressCallback &progressCallback = ProgressCallback());

        /**
         * unlink just one path (no recursion)
         **/
        virtual void unlinkHelper(const FString& path);
    };
};
#endif
