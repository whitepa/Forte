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

        bool FileExists(const FString& path);
        map<FString, bool> m_fileExistsResultMap;
        void setFileExistsResult(const FString& path, bool result);


    protected:
        StrStrMap m_files;
    };
}

#endif
