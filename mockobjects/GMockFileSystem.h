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

        MOCK_METHOD2(Basename, FString (const FString& filename,
                                        const FString& suffix));
        MOCK_METHOD1(Dirname, FString (const FString& filename));
        MOCK_METHOD0(GetCWD, FString ());
        MOCK_METHOD0(GetPathToCurrentProcess, FString ());
        MOCK_CONST_METHOD3(Glob, unsigned int (std::vector<FString> &resultVec,
                                               const FString &pattern,
                                               const int globFlags));
        MOCK_METHOD1(Touch, void (const FString& file));
        MOCK_CONST_METHOD1(FileExists, bool (const FString& filename));
        MOCK_METHOD2(StatFS, void (const FString& path, struct statfs *st));
        MOCK_METHOD2(Stat, int (const FString& path, struct stat *st));
        MOCK_CONST_METHOD1(IsDir, bool (const FString& path));
        MOCK_CONST_METHOD5(GetChildren,
                           void (const FString& path,
                                 std::vector<Forte::FString> &children,
                                 bool recurse,
                                 bool includePathInChildNames,
                                 bool includePathNames));
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
        MOCK_METHOD3(ChangeOwner, void (const FString& path, uid_t uid, gid_t gid));

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
        MOCK_CONST_METHOD2(Truncate, void(const FString& path, off_t size));
    };

    typedef boost::shared_ptr<GMockFileSystem> GMockFileSystemPtr;
};
#endif
