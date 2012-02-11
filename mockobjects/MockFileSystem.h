#ifndef _MockFileSystem_h
#define _MockFileSystem_h

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
        map<int, FString> mFilesByFD;
        void SetFDForFileOpen(unsigned int fd);
        virtual void FileOpen(AutoFD &autoFd, const FString& path, int flags,
                              int mode);
        virtual void FilePutContents(const FString &path, int flags,
                int mode, const char *fmt, ...) __attribute__((format(printf, 5, 6)));

        void FileAppend(const FString& from, const FString& to);

        void clearFileMap();
        StrStrMap* getFileMap();

        int Stat(const Forte::FString& path, struct stat *st);
        map<Forte::FString, int> mStatResultMap;
        map<Forte::FString, struct stat> mStatMap;
        void SetStatResult(const Forte::FString& path, struct stat st, int result);

        void MakeDir(const Forte::FString& path, mode_t mode = 0777, bool make_parents = false);
        vector<Forte::FString> mDirsCreated;
        void ClearDirsCreated();
        vector<Forte::FString> GetDirsCreated();
        bool DirWasCreated(const Forte::FString& path);

        void FileCopy(const Forte::FString& from,
                      const Forte::FString& to, mode_t mode = 0777);
        void ClearCopiedFileMap();
        StrStrMap* GetCopiedFileMap();
        StrStrMap mCopiedFiles; //from -> to
        bool FileWasCopied(const FString& key);

        bool FileExists(const Forte::FString& path) const;
        map<Forte::FString, bool> mFileExistsResultMap;
        void SetFileExistsResult(const Forte::FString& path, bool result);

        bool IsDir(const Forte::FString& path);
		map<Forte::FString, bool> mIsDirResultMap;
		void SetIsDirResult(const Forte::FString& path, bool result);


        int ScanDir(const FString& path, vector<FString> *namelist);
        map<Forte::FString, vector<FString> > m_scanDirResultsMap;
        void AddScanDirResult(const FString& path, FString name);

        void AddDirectoryPathToFileSystem(const FString& path);
        void AddFileToFileSystem(const FString& path, bool createPath);

        void SymLink(const FString& from, const FString& to);
        bool SymLinkWasCreated(const FString& from, const FString& to);
        StrStrMap mSymLinksCreated;

        void Unlink(const FString& path, bool unlink_children = false,
                    const ProgressCallback &progressCallback = ProgressCallback());
        map<FString, bool> mFilesUnlinked;
        bool FileWasUnlinked(const FString& path);
        void ClearFilesUnlinked();

        map<FString, vector<FString> > mChildren;
        void SetChildren(const FString& parentPath,
                         std::vector<Forte::FString> &children);
        void AddChild(const FString& parentPath, const FString& child);
        void RemoveChild(const FString& parentPath, const FString& child);
        void GetChildren(const FString& path,
                         std::vector<Forte::FString> &children,
                         bool recurse = false,
                         bool includePathInChildNames = true) const;


        map<FString, FString> mReadLinkResults;
        virtual FString ReadLink(const FString& path);
        void SetReadLinkResult(const FString& path, const FString& result);
        void ClearReadLinkResult(const FString& path);

    protected:
        StrStrMap mFiles;
        std::map<FString, struct statfs> mStatFSResponseMap;
        unsigned int mFDForFileOpen;
    };
    typedef boost::shared_ptr<MockFileSystem> MockFileSystemPtr;
}

#endif
