#include "MockFileSystem.h"
#include "FTrace.h"
#include "SystemCallUtil.h"

using namespace Forte;

void MockFileSystem::SetStatFSResponse(const FString& path, struct statfs *st)
{
    mStatFSResponseMap[path] = *st;
}

void MockFileSystem::ClearStatFSReponse(const FString& path)
{
    mStatFSResponseMap.erase(path);
}

void MockFileSystem::ClearStatFSResponseAll(void)
{
    mStatFSResponseMap.clear();
}

void MockFileSystem::StatFS(const FString& path, struct statfs *st)
{
    if (st == NULL)
    {
        return;
    }

    if (mStatFSResponseMap.find(path) == mStatFSResponseMap.end())
    {
        // could not find the path, so return a SystemError
        SystemCallUtil::ThrowErrNoException(ENOENT);
    }

    *st = mStatFSResponseMap[path];
}

FString MockFileSystem::FileGetContents(const FString& filename) const
{
    FTRACE2("%s", filename.c_str());
    StrStrMap::const_iterator it = mFiles.find(filename);

    if (it != mFiles.end())
    {
        return it->second;
    }
    else
    {
        return "";
    }
}

void MockFileSystem::FilePutContents(const FString& filename, 
                                     const FString& data,
                                     bool append,
                                     bool throwOnError)
{
    mFiles[filename] = data;
}

void MockFileSystem::FileClearContents(const Forte::FString& filename)
{
    FTRACE2("%s", filename.c_str());

    StrStrMap::iterator it = mFiles.find(filename);

    if (it != mFiles.end())
    {
        hlog(HLOG_DEBUG, "Clearing out file contents in %s", filename.c_str());
        mFiles.erase(it);
    }
}

void MockFileSystem::SetFDForFileOpen(unsigned int fd)
{
    mFDForFileOpen=fd;
}

void MockFileSystem::FileOpen(AutoFD &autoFd, const FString &path, int flags, 
                              int mode)
{
    FTRACE2("%s, %i, %i", path.c_str(), flags, mode);
    
    // don't do anything for now.  we'll need a MockAutoFD or something like that
    autoFd = mFDForFileOpen;
}

void MockFileSystem::FilePutContents(int fd, const char *fmt, ...)
{
    FTRACE2("%i", fd);

    char contents[1024];
    va_list args;
    va_start(args, fmt);
    if (vsnprintf(contents, sizeof(contents), fmt, args) < 0)
    {
        va_end(args);
        hlog_and_throw(HLOG_ERR, Exception("Unable to add file contents"));
    }
    va_end(args);
    
    mFilesByFD[fd] = FString(contents);
    hlog(HLOG_DEBUG, "contents of file is %s", contents);
}


void MockFileSystem::FileAppend(const FString& from, const FString& to)
{
    FString stmp;
    StrStrMap::iterator fromIterator = mFiles.find(from);

    if (fromIterator != mFiles.end())
        mFiles[to].append(fromIterator->second);
    else
    {
        stmp.Format("FORTE_CONCATONATE_FAIL|||%s|||%s", from.c_str(), to.c_str());
        throw EFileSystemAppend(stmp);
    }
}

void MockFileSystem::clearFileMap()
{
    mFiles.clear();
}
StrStrMap* MockFileSystem::getFileMap()
{
    return &mFiles;
}

int MockFileSystem::Stat(const FString& path, struct stat *st)
{

    hlog(HLOG_DEBUG4, "MockFileSystem::%s(%s, [stat])", __FUNCTION__, path.c_str());
    map<FString, struct stat>::iterator stat_it;
    stat_it = mStatMap.find(path);
    if (stat_it == mStatMap.end())
    {
        return 1;
    }

    st->st_nlink = stat_it->second.st_nlink;
    return mStatResultMap[path];
}

void MockFileSystem::SetStatResult(const FString& path, struct stat st, int result)
{

    mStatMap[path] = st;
    mStatResultMap[path] = result;
}

void MockFileSystem::MakeDir(const FString& path, mode_t mode, bool make_parents)
{
    hlog(HLOG_DEBUG4, "MockFileSystem::%s(%s, ...)", __FUNCTION__, path.c_str());
    mDirsCreated.push_back(path);
}
void MockFileSystem::ClearDirsCreated()
{
    mDirsCreated.clear();
}
vector<FString> MockFileSystem::GetDirsCreated()
{
    return mDirsCreated;
}
bool MockFileSystem::DirWasCreated(const FString& path)
{
    if (find(mDirsCreated.begin(),
             mDirsCreated.end(),
             path) == mDirsCreated.end())
    {
        return false;
    }
    return true;
}

void MockFileSystem::FileCopy(const FString& from, const FString& to, mode_t mode)
{
    mCopiedFiles[from] = to;
}
bool MockFileSystem::FileWasCopied(const FString& key)
{
    return mCopiedFiles.find(key) != mCopiedFiles.end();
}
void MockFileSystem::ClearCopiedFileMap()
{
    mCopiedFiles.clear();
}
StrStrMap* MockFileSystem::GetCopiedFileMap()
{
    return &mCopiedFiles;
}

bool MockFileSystem::FileExists(const FString& path) const
{
    FTRACE2("%s", path.c_str());
    map<FString, bool>::const_iterator i;

    if ((i = mFileExistsResultMap.find(path)) != mFileExistsResultMap.end())
    {
        return i->second;
    }

    return false;
}

void MockFileSystem::SetFileExistsResult(const FString& path, bool result)
{
    mFileExistsResultMap[path] = result;
}

FString MockFileSystem::ReadLink(const FString& path)
{
    FTRACE2("%s", path.c_str());

    map<FString, FString>::const_iterator i;

    if ((i = mReadLinkResults.find(path)) != mReadLinkResults.end())
    {
        return i->second;
    }

    throw EErrNoEINVAL(FStringFC(), "%s is not a symlink", path.c_str());
}

void MockFileSystem::SetReadLinkResult(const FString& path, 
                                       const FString& result)
{
    FTRACE2("%s --> %s", path.c_str(), result.c_str());
    mReadLinkResults[path] = result;
}

void MockFileSystem::ClearReadLinkResult(const FString& path)
{
    FTRACE2("%s", path.c_str());

    map<FString, FString>::iterator i;

    if ((i = mReadLinkResults.find(path)) != mReadLinkResults.end())
    {
        hlog(HLOG_DEBUG, "Removing read link result %s->%s",
             path.c_str(), ((*i).second).c_str());
        mReadLinkResults.erase(i);
    }
}

bool MockFileSystem::IsDir(const FString& path)
{
    return mIsDirResultMap[path];
}

void MockFileSystem::SetIsDirResult(const FString& path, bool result)
{
    mIsDirResultMap[path] = result;
}


int MockFileSystem::ScanDir(const FString& path, vector<FString> *namelist)
{



	*namelist = m_scanDirResultsMap[path];

	return (int)m_scanDirResultsMap[path].size();
}


void MockFileSystem::AddScanDirResult(const FString& path, FString name)
{
	hlog(HLOG_DEBUG4, "AddScanDirResult: %s", path.c_str());

	m_scanDirResultsMap[path].push_back(name);

	hlog(HLOG_DEBUG4, "AddScanDirResult resulting size: %d", (int)m_scanDirResultsMap[path].size());

}


void MockFileSystem::AddDirectoryPathToFileSystem(const FString& path)
{


	FString curPath = "";

	hlog(HLOG_DEBUG4, "MockFileSystem::%s(%s)", __FUNCTION__, path.c_str());

	vector<FString> pathsplit = path.split("/");
	int curindex = 0;
	foreach(FString curSplit, pathsplit)
	{
		hlog(HLOG_DEBUG4, "curPath: %s", curPath.c_str());
		hlog(HLOG_DEBUG4, "curSplit: %s", curSplit.c_str());
		if (curPath == "" || curPath.substr(curPath.size() -1 ,1) != "/")
		{
			curPath.append("/");
		}
		curPath.append(curSplit);

		if (curindex < ((int)pathsplit.size() - 1))
		{

			AddScanDirResult(curPath, pathsplit[curindex+1]);

			curindex++;


		}

		//setup filexists
		SetFileExistsResult(curPath,true);

		//setup stat
		struct stat newst;
		newst.st_ino = mStatMap.size() + 1223;
		newst.st_nlink = 1;

		SetStatResult(curPath,newst,true);
		SetIsDirResult(curPath,true);



	}

}
void MockFileSystem::AddFileToFileSystem(const FString& path, bool createPath)
{
	//do a foreach on the split of the path
	FString curPath;
	hlog(HLOG_DEBUG4, "MockFileSystem::%s(%s)", __FUNCTION__, path.c_str());

	foreach(FString curSplit, path.split("/"))
	{
		if (curSplit != "")
		{


			curPath.append("/");
			curPath.append(curSplit.c_str());

			hlog(HLOG_DEBUG4, "curPath: %s", curPath.c_str());
			hlog(HLOG_DEBUG4, "curSplit: %s", curSplit.c_str());
			//setup filexists
			SetFileExistsResult(curPath,true);

			//setup stat

			//setup scan dir

		}

	}
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
        const ProgressCallback &progressCallback)
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

void MockFileSystem::AddChild(const FString& parentPath, const FString& child)
{
    FTRACE2("%s, %s", parentPath.c_str(), child.c_str());

    mChildren[parentPath].push_back(child);
}

void MockFileSystem::RemoveChild(const FString& parentPath, 
                                 const FString& child)
{
    FTRACE2("%s, %s", parentPath.c_str(), child.c_str());

    std::map<FString, std::vector<FString> >::iterator it;
    std::vector<FString>::iterator childIt;
    if ((it = mChildren.find(parentPath)) == mChildren.end())
    {
        return;
    }

    std::vector<FString>& children=(*it).second;
    childIt=std::find<std::vector<FString>::iterator, FString>(children.begin(), children.end(), child);
    if (childIt != children.end())
    {
        hlog(HLOG_DEBUG, "Removing %s from %s", child.c_str(), 
             parentPath.c_str());
        children.erase(childIt);
    }
}

void MockFileSystem::SetChildren(const FString &parentPath,
                                 std::vector<FString> &children)
{
    FTRACE2("%s", parentPath.c_str());

    foreach (const FString &child, children)
    {
        mChildren[parentPath].push_back(child);
    }
}

void MockFileSystem::GetChildren(const FString& path, 
                                 std::vector<Forte::FString> &children,
                                 bool recurse, 
                                 bool includePathInChildNames) const
{
    FTRACE2("%s", path.c_str());
    map<FString, vector<FString> >::const_iterator i;

    if ((i = mChildren.find(path)) == mChildren.end())
    {
        return;
    }
    
    foreach (const FString &child, i->second)
    {
        children.push_back(child);
    }
}
