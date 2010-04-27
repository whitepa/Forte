#include "MockFileSystem.h"

using namespace Forte;

void MockFileSystem::StatFS(const FString& path, struct statfs *st)
{
    //TODO: make this settable and gettable
    if (st == NULL)
    {
        return;
    }

    st->f_blocks = 100;
    st->f_bfree = 50;
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
