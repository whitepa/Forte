#include "MockFileSystem.h"
#include "FTrace.h"

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


FString MockFileSystem::FileGetContents(const FString& filename)
{
    return m_files[filename];
}

void MockFileSystem::FilePutContents(const FString& filename, 
                                     const FString& data,
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

bool MockFileSystem::FileExists(const FString& path)
{
    return FileExists(path);
}

bool MockFileSystem::FileExists(const FString& path)
{
    return m_fileExistsResultMap[path];
}

void MockFileSystem::setFileExistsResult(const FString& path, bool result)
{
    m_fileExistsResultMap[path] = result;
}

void MockFileSystem::SymLink(const FString& from, const FString& to)
{
    mSymLinksCreated[from] = to;
}

bool MockFileSystem::SymLinkWasCreated(const FString& from, const FString& to)
{
    FTRACE2("%s -> %s", from.c_str(), to.c_str());

    StrStrMap::const_iterator i;
    if ((i = mSymLinksCreated.find(from)) == mSymLinksCreated.end())
    {
        return false;
    } 
    else if ((*i).second == to) // make sure the to matches
    {
        return true;
    }

    return false;
}

void MockFileSystem::Unlink(const FString& path, bool unlink_children,
                    progress_callback_t progress_callback,
                    void *callback_data)
{
    FTRACE2("%s", path.c_str());

    mFilesUnlinked[path] = true;
}

void MockFileSystem::ClearFilesUnlinked()
{
    mFilesUnlinked.clear();
}

bool MockFileSystem::FileWasUnlinked(const FString& path)
{
    FTRACE2("%s", path.c_str());

    return (mFilesUnlinked.find(path) != mFilesUnlinked.end());
}
