#ifndef _GMockFileSystem_h
#define _GMockFileSystem_h

#include "FileSystem.h"
#include <gmock/gmock.h>

namespace Forte
{
    class GMockFileSystem : public FileSystem
    {
    public:
        GMockFileSystem() {}
        ~GMockFileSystem() {}


/*
        FString Basename(const FString& filename, const FString& suffix = "");
        FString Dirname(const FString& filename);
        FString GetCWD();
        void Touch(const FString& file);
        bool FileExists(const FString& filename) const;
        void StatFS(const FString& path, struct statfs *st);
        int Stat(const FString& path, struct stat *st);
        bool IsDir(const FString& path) const;
        void GetChildren(const FString& path, std::vector<Forte::FString> &children, bool recurse = false, bool includePathInChildNames = true) const;
        uint64_t CountChildren(const FString& path, bool recurse) const;
        int LStat(const FString& path, struct stat *st);
        int StatAt(int dir_fd, const FString& path, struct stat *st);
        int LStatAt(int dir_fd, const FString& path, struct stat *st);
        int FStatAt(int dir_fd, const FString& path, struct stat *st, int flags = 0);
        void Unlink(const FString& path, bool unlink_children = false, const ProgressCallback &progressCallback = ProgressCallback());
        void UnlinkAt(int dir_fd, const FString& path);
        void Rename(const FString& from, const FString& to);
        void RenameAt(int dir_from_fd, const FString& from, int dir_to_fd, const FString& to);
        void MakeDir(const FString& path, mode_t mode = 0777, bool make_parents = false);
        void MakeDirAt(int dir_fd, const FString& path, mode_t mode = 0777);
        void MakeFullPath(const FString& path, mode_t mode = 0777);
        FString MakeRelativePath(const FString& base, const FString& path);
        FString ResolveRelativePath(const FString& base, const FString& path);
        int ScanDir(const FString& path, std::vector<FString> &namelist);
        void Link(const FString& from, const FString& to);
        void LinkAt(int dir_from_fd, const FString& from, int dir_to_fd, const FString& to);
        void SymLink(const FString& from, const FString& to);
        void SymLinkAt(const FString& from, intdir_to_fd, const FString& to);
        FString ReadLink(const FString& path);
        FString ResolveSymLink(const FString& path);
        FString FullyResolveSymLink(const FString& path);
        FString FileGetContents(const FString& filename) const;
        void FilePutContents(const FString& filename, const FString& data, bool append=false, bool throwOnError=false);
        void FileOpen(AutoFD &autoFd, const FString &path, int flags, int mode);
        void FilePutContents(const FString &path, int flags, int mode, const char *fmt, ...) __attribute__((format(printf, 5, 6)));
        void FileAppend(const FString& from, const FString& to);
        void FileCopy(const FString& from, const FString& to, mode_t mode = 0777);
        void DeepCopy(const FString& source, const FString& dest, const ProgressCallback &progressCallback = ProgressCallback());
        void Copy(const FString& from_path, const FString& to_path, const ProgressCallback &progressCallback = ProgressCallback());
        FString StrError(int err) const;
*/


        MOCK_METHOD2(Basename, FString (const FString& filename,
                                        const FString& suffix));
        MOCK_METHOD1(Dirname, FString (const FString& filename));
        MOCK_METHOD0(GetCWD, FString ());
        MOCK_METHOD0(GetPathToCurrentProcess, FString ());
        MOCK_METHOD1(Touch, void (const FString& file));
        MOCK_CONST_METHOD1(FileExists, bool (const FString& filename));
        MOCK_METHOD2(StatFS, void (const FString& path, struct statfs *st));
        MOCK_METHOD2(Stat, int (const FString& path, struct stat *st));
        MOCK_CONST_METHOD1(IsDir, bool (const FString& path));
        MOCK_CONST_METHOD4(GetChildren,
                           void (const FString& path,
                                 std::vector<Forte::FString> &children,
                                 bool recurse,
                                 bool includePathInChildNames));
        MOCK_CONST_METHOD2(CountChildren, uint64_t (const FString& path, bool recurse));
        MOCK_METHOD2(LStat, int (const FString& path, struct stat *st));
        MOCK_METHOD3(StatAt, int (int dir_fd, const FString& path, struct stat *st));
        MOCK_METHOD3(LStatAt, int (int dir_fd, const FString& path, struct stat *st));
        MOCK_METHOD4(FStatAt, int (int dir_fd, const FString& path, struct stat *st, int flags));
        MOCK_METHOD3(Unlink, void (const FString& path, bool unlink_children,
                                   const ProgressCallback &progressCallback));
        MOCK_METHOD2(UnlinkAt, void (int dir_fd, const FString& path));
        MOCK_METHOD2(Rename, void (const FString& from, const FString& to));
        MOCK_METHOD4(RenameAt, void (int dir_from_fd, const FString& from, int dir_to_fd, const FString& to));
        MOCK_METHOD3(MakeDir,
                     void (const FString& path, mode_t mode, bool make_parents));
        MOCK_METHOD3(MakeDirAt, void (int dir_fd, const FString& path, mode_t mode));
        MOCK_METHOD2(MakeFullPath, void (const FString& path, mode_t mode));
        MOCK_METHOD2(MakeRelativePath, FString (const FString& base, const FString& path));
        MOCK_METHOD2(ResolveRelativePath, FString (const FString& base, const FString& path));
        MOCK_METHOD2(ScanDir, int (const FString& path, std::vector<FString> &namelist));
        MOCK_METHOD2(Link, void (const FString& from, const FString& to));
        MOCK_METHOD4(LinkAt, void (int dir_from_fd, const FString& from, int dir_to_fd, const FString& to));
        MOCK_METHOD2(SymLink, void (const FString& from, const FString& to));
        MOCK_METHOD3(SymLinkAt, void (const FString& from, int dir_to_fd, const FString& to));
        MOCK_METHOD1(ReadLink, FString (const FString& path));
        MOCK_METHOD1(ResolveSymLink, FString (const FString& path));
        MOCK_METHOD1(FullyResolveSymLink, FString (const FString& path));
        MOCK_CONST_METHOD1(FileGetContents, FString (const FString& filename));
        MOCK_METHOD4(FilePutContents,
                     void (const FString& filename, const FString& data,
                           bool append, bool throwOnError));
        MOCK_METHOD4(FilePutContentsWithPerms, void(
                         const FString& filename,
                         const FString& data,
                         int flags,
                         int mode));
        MOCK_METHOD4(FileOpen, void (AutoFD &autoFd, const FString &path, int flags, int mode));
        MOCK_METHOD2(FileAppend, void (const FString& from, const FString& to));
        MOCK_METHOD3(FileCopy, void (const FString& from, const FString& to, mode_t mode));
        MOCK_METHOD3(DeepCopy, void (const FString& source, const FString& dest, const ProgressCallback &progressCallback));
        MOCK_METHOD3(Copy, void (const FString& from_path, const FString& to_path, const ProgressCallback &progressCallback));
        MOCK_CONST_METHOD1(StrError, FString (int err));
        MOCK_CONST_METHOD1(MakeTemporaryFile, FString(const FString&));
    };

    typedef boost::shared_ptr<GMockFileSystem> GMockFileSystemPtr;
};
#endif
