#ifndef _MockFileSystem_h
#define _MockFileSystem_h

#include "FileSystem.h"

namespace Forte
{
    class MockFileSystem : public FileSystem
    {
    public:
        static void SetupSingleton();
        static void RealSingleton();
    public:
        // mock stuff:
        //mocked up function
        //helper functions
        //helper data

        FString file_get_contents(const FString& filename);
        void file_put_contents(const FString& filename, const FString& data, bool append=false);
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
        map<FString, bool> m_fileExistsResultMap;
        void setFileExistsResult(const FString& path, bool result);

        /*
          FString getcwd();
          void touch(const FString& file);
          int lstat(const FString& path, struct stat *st);
          void unlink(const FString& path, bool unlink_children = false);
          void rename(const FString& from, const FString& to);
          inline void mkdir_all(const FString& path, mode_t mode = 0777) { mkdir(path, mode, true); }
          void link(const FString& from, const FString& to);
          void symlink(const FString& from, const FString& to);
          FString readlink(const FString& path);
          FString resolve_symlink(const FString& path);
          FString fully_resolve_symlink(const FString& path);
          FString make_rel_path(const FString& base, const FString& path);
          FString resolve_rel_path(const FString& base, const FString& path);
        */
    protected:

        StrStrMap m_files;
        static FileSystem *s_real_fs;
        static MockFileSystem *s_mock_fs;

    };
}

#endif
