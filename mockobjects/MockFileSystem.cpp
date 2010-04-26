#include "MockFileSystem.h"

using namespace Forte;

FileSystem * MockFileSystem::s_real_fs = NULL;
MockFileSystem * MockFileSystem::s_mock_fs = NULL;

void MockFileSystem::SetupSingleton()
{
    get(); // make sure we create a real singleton first
    CAutoUnlockMutex lock(FileSystem::sMutex);
    if (s_real_fs == NULL) // must be a real FS in the singleton
        s_real_fs = FileSystem::sSingleton;
    if (s_mock_fs == NULL)
        s_mock_fs = new MockFileSystem();
    FileSystem::sSingleton = s_mock_fs;
}

void MockFileSystem::RealSingleton()
{
    get(); // make sure we create a real singleton first
    CAutoUnlockMutex lock(FileSystem::sMutex);
    if (s_real_fs == NULL) // must be a real FS in the singleton
        s_real_fs = FileSystem::sSingleton;
    else
        FileSystem::sSingleton = s_real_fs;
}


FString MockFileSystem::file_get_contents(const FString& filename)
{
    return m_files[filename];
}


void MockFileSystem::file_put_contents(const FString& filename, const FString& data,
				       bool append)
{
    m_files[filename] = data;
}

void MockFileSystem::clearFileMap()
{
    m_files.clear();
}
StrStrMap* MockFileSystem::getFileMap()
{
    return &m_files;
}

int MockFileSystem::stat(const FString& path, struct stat *st)
{
    hlog(HLOG_DEBUG4, "MockFileSystem::%s(%s, [stat])", __FUNCTION__, path.c_str());
    map<FString, struct stat>::iterator stat_it;
    stat_it = m_statMap.find(path);
    if (stat_it == m_statMap.end())
    {
        return 1;
    }

    st->st_nlink = stat_it->second.st_nlink;
    return m_statResultMap[path];
}

void MockFileSystem::setStatResult(const FString& path, struct stat st, int result)
{
    m_statMap[path] = st;
    m_statResultMap[path] = result;
}

void MockFileSystem::mkdir(const FString& path, mode_t mode, bool make_parents)
{
    hlog(HLOG_DEBUG4, "MockFileSystem::%s(%s, ...)", __FUNCTION__, path.c_str());
    m_dirsCreated.push_back(path);
}

vector<FString> MockFileSystem::getDirsCreated()
{
    return m_dirsCreated;
}

bool MockFileSystem::dirWasCreated(const FString& path)
{
    if (find(m_dirsCreated.begin(),
             m_dirsCreated.end(),
             path) == m_dirsCreated.end())
    {
        return false;
    }
    return true;
}

void MockFileSystem::file_copy(const FString& from, const FString& to, mode_t mode)
{
    m_copiedFiles[from] = to;
}
void MockFileSystem::clearCopiedFileMap()
{
    m_copiedFiles.clear();
}
StrStrMap* MockFileSystem::getCopiedFileMap()
{
    return &m_copiedFiles;
}

bool MockFileSystem::file_exists(const FString& path)
{
    return m_fileExistsResultMap[path];
}

void MockFileSystem::setFileExistsResult(const FString& path, bool result)
{
    m_fileExistsResultMap[path] = result;
}
