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


FString MockFileSystem::FileGetContents(const FString& filename)
{
    return mFiles[filename];
}

void MockFileSystem::FilePutContents(const FString& filename, 
                                     const FString& data,
                                     bool append)
{
    mFiles[filename] = data;
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

void MockFileSystem::FileCopy(ProcRunner &pr, const FString& from, const FString& to, mode_t mode)
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

bool MockFileSystem::FileExists(const FString& path)
{
    return mFileExistsResultMap[path];
}

void MockFileSystem::SetFileExistsResult(const FString& path, bool result)
{
    mFileExistsResultMap[path] = result;
}

bool MockFileSystem::IsDir(const FString& path)
{
    return mIsDirResultMap[path];
}

void MockFileSystem::SetIsDirResult(const FString& path, bool result)
{
    mIsDirResultMap[path] = result;
}


int MockFileSystem::ScanDir(const FString& path, vector<FString> *namelist, int(*compar)(const void *, const void *))
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


