#ifndef _MockFileSystem_h
#define _MockFileSystem_h

#include "FileSystem.h"

namespace Forte
{
    class MockFileSystem : public FileSystem
    {
    public:
        void StatFS(const FString& path, struct statfs *st);

        FString FileGetContents(const FString& filename);
        void FilePutContents(const FString& filename, 
                             const FString& data, 
                             bool append=false);

        void clearFileMap();
        StrStrMap* getFileMap();

        int stat(const FString& path, struct stat *st);
        map<FString, int> m_statResultMap;
        map<FString, struct stat> m_statMap;
        void setStatResult(const FString& path, struct stat st, int result);

        void mkdir(const FString& path, mode_t mode = 0777, bool make_parents = false);
        vector<FString> m_dirsCreated;
        vector<FString> getDirsCreated();
        bool dirWasCreated(const FString& path);

        void file_copy(const FString& from, const FString& to, mode_t mode = 0777);
        void clearCopiedFileMap();
        StrStrMap* getCopiedFileMap();
        StrStrMap m_copiedFiles; //from -> to

        bool file_exists(const FString& path);
        bool FileExists(const FString& path) const;
        map<FString, bool> m_fileExistsResultMap;
        void setFileExistsResult(const FString& path, bool result);

        void SymLink(const FString& from, const FString& to);
        bool SymLinkWasCreated(const FString& from, const FString& to);
        StrStrMap mSymLinksCreated;
        
        void Unlink(const FString& path, bool unlink_children = false,
                    progress_callback_t progress_callback = NULL,
                    void *callback_data = NULL);
        map<FString, bool> mFilesUnlinked;
        bool FileWasUnlinked(const FString& path);
        void ClearFilesUnlinked();

        map<FString, vector<FString> > mChildren;
        void SetChildren(const FString& parentPath, 
                         std::vector<Forte::FString> &children);

        void GetChildren(const FString& path, 
                         std::vector<Forte::FString> &children,
                         bool recurse = false) const;
        

    protected:
        StrStrMap m_files;
    };
}

#endif
