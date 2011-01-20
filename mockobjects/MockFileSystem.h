#ifndef _MockFileSystem_h
#define _MockFileSystem_h

#include "FileSystem.h"

namespace Forte
{
    class MockFileSystem : public FileSystem
    {
    public:
        void StatFS(const FString& path, struct statfs *st);

        Forte::FString FileGetContents(const Forte::FString& filename);
        void FilePutContents(const Forte::FString& filename, 
                             const Forte::FString& data, 
                             bool append=false);

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

        void FileCopy(ProcRunner &pr, const Forte::FString& from, 
                      const Forte::FString& to, mode_t mode = 0777);
        void ClearCopiedFileMap();
        StrStrMap* GetCopiedFileMap();
        StrStrMap mCopiedFiles; //from -> to
        bool FileWasCopied(const FString& key);


        map<Forte::FString, bool> mFileExistsResultMap;
        void SetFileExistsResult(const Forte::FString& path, bool result);
        void setFileExistsResult(const FString& path, bool result);

        bool IsDir(const Forte::FString& path);
		map<Forte::FString, bool> mIsDirResultMap;
		void SetIsDirResult(const Forte::FString& path, bool result);


        int ScanDir(const FString& path, vector<FString> *namelist);
        map<Forte::FString, vector<FString> > m_scanDirResultsMap;
        void AddScanDirResult(const FString& path, FString name);

        void AddDirectoryPathToFileSystem(const FString& path);
        void AddFileToFileSystem(const FString& path, bool createPath);

        bool file_exists(const FString& path);
        bool FileExists(const FString& path);

        void SymLink(const FString& from, const FString& to);
        bool SymLinkWasCreated(const FString& from, const FString& to);
        StrStrMap mSymLinksCreated;
        
        void Unlink(const FString& path, bool unlink_children = false,
                    progress_callback_t progress_callback = NULL,
                    void *callback_data = NULL);
        map<FString, bool> mFilesUnlinked;
        bool FileWasUnlinked(const FString& path);
        void ClearFilesUnlinked();


    protected:
        StrStrMap mFiles;
    };
}

#endif
