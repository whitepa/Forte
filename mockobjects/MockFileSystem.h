#ifndef _MockFileSystem_h
#define _MockFileSystem_h

#include "FTrace.h"
#include "FileSystem.h"

namespace Forte
{
    class MockFileSystem : public FileSystem
    {
    public:
        void SetStatFSResponse(const FString& path, struct statfs *st);
        void ClearStatFSReponse(const FString& path);
        void ClearStatFSResponseAll(void);
        void StatFS(const FString& path, struct statfs *st);

        Forte::FString FileGetContents(const Forte::FString& filename) const;
        void FilePutContents(const Forte::FString& filename,
                             const Forte::FString& data,
                             bool append=false,
                             bool throwOnError=false);
        void FileClearContents(const Forte::FString& filename);
        std::map<int, FString> mFilesByFD;
        void SetFDForFileOpen(unsigned int fd);
        virtual void FileOpen(AutoFD &autoFd, const FString& path, int flags,
                              int mode);
        void FileAppend(const FString& from, const FString& to);

        void clearFileMap();
        StrStrMap* getFileMap();

        int Stat(const Forte::FString& path, struct stat *st);
        std::map<Forte::FString, int> mStatResultMap;
        std::map<Forte::FString, struct stat> mStatMap;
        void SetStatResult(const Forte::FString& path, struct stat st, int result);

        void MakeDir(const Forte::FString& path, mode_t mode = 0777, bool make_parents = false);
        std::vector<Forte::FString> mDirsCreated;
        void ClearDirsCreated();
        std::vector<Forte::FString> GetDirsCreated();
        bool DirWasCreated(const Forte::FString& path);

        void FileCopy(const Forte::FString& from,
                      const Forte::FString& to, mode_t mode = 0777);
        void ClearCopiedFileMap();
        StrStrMap* GetCopiedFileMap();
        StrStrMap mCopiedFiles; //from -> to
        bool FileWasCopied(const FString& key);

        bool FileExists(const Forte::FString& path) const;
        std::map<Forte::FString, bool> mFileExistsResultMap;
        void SetFileExistsResult(const Forte::FString& path, bool result);

        bool IsDir(const Forte::FString& path) const;
        std::map<Forte::FString, bool> mIsDirResultMap;
                void SetIsDirResult(const Forte::FString& path, bool result);


        int ScanDir(const FString& path, std::vector<FString> &namelist);
        std::map<Forte::FString, std::vector<FString> > m_scanDirResultsMap;
        void AddScanDirResult(const FString& path, FString name);
        void RemoveScanDirResult(const FString& path, FString name);

        void AddDirectoryPathToFileSystem(const FString& path);
        void AddFileToFileSystem(const FString& path, bool createPath);

        void SymLink(const FString& from, const FString& to);
        Forte::FString ResolveSymLink(const FString& from);
        bool SymLinkWasCreated(const FString& from, const FString& to);
        StrStrMap mSymLinksCreated;

        void Unlink(const FString& path, bool unlink_children = false,
                    const ProgressCallback &progressCallback = ProgressCallback());
        std::map<FString, bool> mFilesUnlinked;
        bool FileWasUnlinked(const FString& path);
        void ClearFilesUnlinked();

        std::map<FString, std::vector<FString> > mChildren;
        void SetChildren(const FString& parentPath,
                         std::vector<Forte::FString> &children);
        void AddChild(const FString& parentPath, const FString& child);
        void RemoveChild(const FString& parentPath, const FString& child);
        void GetChildren(const FString& path,
                         std::vector<Forte::FString> &children,
                         bool recurse = false,
                         bool includePathInChildNames = true,
                         bool includePathNames = false) const;

        void SetGlobResponse(const FString &pattern,
                             const std::vector<FString> &response);
        void ClearGlobResponse(const FString& pattern = "");
        unsigned int Glob(std::vector<FString> &resultVec,
                          const FString &pattern,
                          const int globFlags = 0) const;

        std::map<FString, FString> mReadLinkResults;
        virtual FString ReadLink(const FString& path);
        void SetReadLinkResult(const FString& path, const FString& result);
        void ClearReadLinkResult(const FString& path);

        FString Basename(const FString& filename, const FString& suffix = "");

        FString Dirname(const FString& filename);

        FString GetCWD()
            { FTRACE; throw EUnimplemented(); }
        FString GetPathToCurrentProcess()
            { FTRACE; throw EUnimplemented(); }
        void Touch(const FString& file)
            { FTRACE; throw EUnimplemented(); }
        uint64_t CountChildren(const FString& path, bool recurse) const
            { FTRACE; throw EUnimplemented(); }
        int LStat(const FString& path, struct stat *st)
            { FTRACE; throw EUnimplemented(); }
        int StatAt(int dir_fd, const FString& path, struct stat *st)
            { FTRACE; throw EUnimplemented(); }
        int LStatAt(int dir_fd, const FString& path, struct stat *st)
            { FTRACE; throw EUnimplemented(); }
        int FStatAt(int dir_fd, const FString& path, struct stat *st, int flags = 0)
            { FTRACE; throw EUnimplemented(); }
        void UnlinkAt(int dir_fd, const FString& path)
            { FTRACE; throw EUnimplemented(); }
        void Rename(const FString& from, const FString& to)
            { FTRACE; throw EUnimplemented(); }
        void RenameAt(int dir_from_fd, const FString& from, int dir_to_fd, const FString& to)
            { FTRACE; throw EUnimplemented(); }
        void ChangeOwner(const FString& path, uid_t uid, gid_t gid)
            { FTRACE; throw EUnimplemented(); }
        void MakeDirAt(int dir_fd, const FString& path, mode_t mode = 0777)
            { FTRACE; throw EUnimplemented(); }
        void MakeFullPath(const FString& path, mode_t mode = 0777)
            { FTRACE; throw EUnimplemented(); }
        FString MakeRelativePath(const FString& base, const FString& path)
            { FTRACE; throw EUnimplemented(); }
        FString ResolveRelativePath(const FString& base, const FString& path)
            { FTRACE; throw EUnimplemented(); }
        void Link(const FString& from, const FString& to)
            { FTRACE; throw EUnimplemented(); }
        void LinkAt(int dir_from_fd, const FString& from, int dir_to_fd, const FString& to)
            { FTRACE; throw EUnimplemented(); }
        void SymLinkAt(const FString& from, int dir_to_fd, const FString& to)
            { FTRACE; throw EUnimplemented(); }
        FString FullyResolveSymLink(const FString& path)
            { FTRACE; throw EUnimplemented(); }
        void DeepCopy(const FString& source, const FString& dest, const ProgressCallback &progressCallback = ProgressCallback())
            { FTRACE; throw EUnimplemented(); }
        void Copy(const FString& from_path, const FString& to_path, const ProgressCallback &progressCallback = ProgressCallback())
            { FTRACE; throw EUnimplemented(); }
        FString StrError(int err) const
            { FTRACE; throw EUnimplemented(); }

        virtual void FilePutContentsWithPerms(
            const FString& filename,
            const FString& data,
            int flags,
            int mode)
            { FTRACE; throw EUnimplemented(); }

        virtual FString MakeTemporaryFile(const FString& nameTemplate) const {
            FTRACE;
            throw EUnimplemented();
        }

        virtual void Truncate(const FString& path, off_t size) const {
            FTRACE;
            throw EUnimplemented();
        }

    protected:
        StrStrMap mFiles;
        std::map<FString, struct statfs> mStatFSResponseMap;
        std::map<FString, std::vector<FString> > mGlobResponseMap;
        unsigned int mFDForFileOpen;
    };
    typedef boost::shared_ptr<MockFileSystem> MockFileSystemPtr;
}

#endif
