// FileSystem.cpp

#include "FileSystem.h"
#include "LogManager.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <vector>
#include <cstring>
#include <sys/time.h>
#include "SystemCallUtil.h"

using namespace Forte;
namespace boostfs = boost::filesystem;

// constants
const unsigned MAX_RESOLVE = 1000;

// ctor/dtor
FileSystem::FileSystem()
{
}


FileSystem::~FileSystem()
{
}

// helpers
FString FileSystem::StrError(int err) const
{
    char buf[256], *str;
    FString ret;
    str = strerror_r(err, buf, sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    ret = str;
    return ret;
}


// interface
FString FileSystem::GetCWD()
{
    FString ret;
    char buf[1024];  // MAXPATHLEN + 1

    ::getcwd(buf, sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    ret = buf;
    return ret;
}


void FileSystem::Touch(const FString& file)
{
    AutoFD fd;
    struct timeval tv[2];

    if ((fd = ::open(file, O_WRONLY | O_CREAT, 0666)) == AutoFD::NONE)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }

    if (gettimeofday(&(tv[0]), NULL) == -1)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }

    memcpy(&(tv[1]), &(tv[0]), sizeof(tv[0]));

    if (::futimes(fd, tv) == -1)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }
}


bool FileSystem::FileExists(const FString& filename) const
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, filename.c_str());
    struct stat st;
    // NOTE: I think this will fix a sporadic bug seen on GPFS filesystems
    //       where we are deleting an iSCSI LUN file (i.e. moving it to the
    //       trash), then creating a new LUN file with the same name.  The
    //       second creation fails because it thinks the file exists.
    // TODO: investigate why/if we need two stat() calls here.
    stat(filename, &st);
    return (stat(filename, &st) == 0);
}

bool FileSystem::IsDir(const FString& path) const
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    struct stat st;
    
    if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void FileSystem::StatFS(const FString& path, struct statfs *st)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    //TODO: check return codes, return appropriate values

    if (::statfs(path.c_str(), st) == 0)
    {
        return;
    }

    SystemCallUtil::ThrowErrNoException(errno);
}

int FileSystem::Stat(const FString& path, struct stat *st)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    return ::stat(path.c_str(), st);
}


int FileSystem::LStat(const FString& path, struct stat *st)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    return ::lstat(path.c_str(), st);
}


int FileSystem::StatAt(int dir_fd, const FString& path, struct stat *st)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s)", __FUNCTION__, dir_fd, path.c_str());
    return ::fstatat(dir_fd, path.c_str(), st, 0);
}


int FileSystem::LStatAt(int dir_fd, const FString& path, struct stat *st)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s)", __FUNCTION__, dir_fd, path.c_str());
    return ::fstatat(dir_fd, path.c_str(), st, AT_SYMLINK_NOFOLLOW);
}


int FileSystem::FStatAt(int dir_fd, const FString& path, struct stat *st, int flags)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s)", __FUNCTION__, dir_fd, path.c_str());
    return ::fstatat(dir_fd, path.c_str(), st, flags);
}

void FileSystem::GetChildren(const FString& path, 
                             std::vector<Forte::FString> &children,
                             bool recurse) const
{

    AutoFD dir = opendir(path);
    if (errno != 0)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }

    int err_number = 0;
    FString stmp;
    struct dirent *result;
    struct dirent entry;

    while ((err_number = readdir_r(dir, &entry, &result)) == 0
           && result != NULL)
    {
        stmp = entry.d_name;
        
        if (stmp != "." && stmp != "..")
        {
            if (IsDir(path + "/" + stmp))
            {
                if (recurse)
                {
                    GetChildren(path + "/" + stmp, children, recurse);
                }
                else
                {
                    hlog(HLOG_DEBUG, "Skipping %s (not recursing)", 
                         stmp.c_str());
                }
            }
            else
            {
                children.push_back(path + "/" + stmp);
            }
        }
    }    
}


void FileSystem::Unlink(const FString& path, bool unlink_children,
                        progress_callback_t progress_callback,
                        void *callback_data)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %s)", __FUNCTION__,
         path.c_str(), (unlink_children ? "true" : "false"));
    FString stmp;

    // recursive delete?
    if (unlink_children)
    {
        int err_number;
        struct dirent entry;
        struct dirent *result;

        struct stat st;
        AutoFD dir;

        // inefficient but effective recursive delete algorithm
        // NOTE: use lstat() so we don't follow symlinks
        if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode))
        {
            errno = 0;
            dir = opendir(path);

            if (dir.dir() == NULL || errno != 0)
            {
                if (errno == ENOENT)
                {
                    // the thing we're trying to unlink is gone, great!
                    return;
                }

                //else, we could not work with this dir to delete it
                SystemCallUtil::ThrowErrNoException(errno);
            }

            err_number = 0;
            while ((err_number = readdir_r(dir, &entry, &result)) == 0
                   && result != NULL)
            {
                stmp = entry.d_name;

                if (stmp != "." && stmp != "..")
                {
                    Unlink(path + "/" + entry.d_name,
                           true, progress_callback, callback_data);
                }
            }

            // when finished we should get NULL for readdir_r and no error.
            if (err_number != 0)
            {
                if (errno == ENOENT)
                {
                    // the thing we're trying to unlink is gone, great!
                    return;
                }

                SystemCallUtil::ThrowErrNoException(errno);
            }
        }
    }

    // delete self
    unlinkHelper(path);

    // callback?
    if (progress_callback != NULL) progress_callback(0, callback_data);
}


void FileSystem::UnlinkAt(int dir_fd, const FString& path)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s)", __FUNCTION__, dir_fd, path.c_str());
    FString stmp;
    int err;

    // delete self
    if ((err = ::unlinkat(dir_fd, path, 0)) != 0)
    {
        if (errno == ENOENT) return;
        if (errno == EISDIR) err = ::unlinkat(dir_fd, path, AT_REMOVEDIR);
    }

    if (err != 0)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }
}


void FileSystem::unlinkHelper(const FString& path)
{
    // hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    int err;

    if ((err = ::unlink(path)) != 0)
    {
        if (errno == ENOENT) return;
        if (errno == EISDIR) err = ::rmdir(path);
    }

    if (err != 0)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }
}


void FileSystem::Rename(const FString& from, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %s)", __FUNCTION__, from.c_str(), to.c_str());

    if (::rename(from.c_str(), to.c_str()) != 0)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }
}


void FileSystem::RenameAt(int dir_from_fd, const FString& from,
                          int dir_to_fd, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s, %d, %s)", __FUNCTION__,
         dir_from_fd, from.c_str(), dir_to_fd, to.c_str());

    if (::renameat(dir_from_fd, from.c_str(), dir_to_fd, to.c_str()) != 0)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }
}


void FileSystem::MakeDir(const FString& path, mode_t mode, bool make_parents)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %04o, %s)", __FUNCTION__,
         path.c_str(), mode, (make_parents ? "true" : "false"));
    FString stmp, parent;
    struct stat st;
    int err;

    // check for /
    if (path.empty() || path == "/") return;

    // path exists?
    if (stat(path, &st) == 0)
    {
        // early exit?
        if (S_ISDIR(st.st_mode)) 
        {
            return;
        }
        else 
        {
            stmp.Format("FORTE_MAKEDIR_FAIL_IN_THE_WAY|||%s", 
                        path.c_str());
            throw EFileSystemMakeDir(stmp);
        }
    }
    else
    {
        // create parent?
        if (make_parents)
        {
            parent = path.Left(path.find_last_of('/'));
            MakeDir(parent, mode, true);
        }

        // make path
        ::mkdir(path, mode);
        err = errno;

        // validate path
        if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode))
        {
            if (err == 0)
            {
                throw EFileSystemMakeDir(FStringFC(), 
                                     "FORTE_MAKEDIR_SUBSEQUENTLY_DELETED|||%s",
                                     path.c_str());
            }

            SystemCallUtil::ThrowErrNoException(err);
        }
    }
}

int FileSystem::ScanDir(const FString& path, std::vector<FString> &namelist)
{
	if (boostfs::exists(path))
	{
		namelist.clear();
		boostfs::directory_iterator end_itr; // default construction yields past-the-end
		  for (boostfs::directory_iterator itr(path);itr != end_itr; ++itr )
		  {

		    namelist.push_back(itr->path().filename());
		  }

	}
	return (int)namelist.size();
}

void FileSystem::MakeDirAt(int dir_fd, const FString& path, mode_t mode)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s, %04o)", __FUNCTION__, dir_fd, path.c_str(), mode);
    struct stat st;
    int err;

    // make path
    ::mkdirat(dir_fd, path, mode);
    err = errno;

    // validate path
    if (StatAt(dir_fd, path, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        if (err == 0)
        {
            throw EFileSystemMakeDir(FStringFC(), 
                                     "FORTE_MAKEDIR_SUBSEQUENTLY_DELETED|||%s",
                                     path.c_str());
        }

        SystemCallUtil::ThrowErrNoException(err);
    }
}


void FileSystem::Link(const FString& from, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %s)", __FUNCTION__, from.c_str(), to.c_str());

    if (::link(from.c_str(), to.c_str()) != 0)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }
}


void FileSystem::LinkAt(int dir_from_fd, const FString& from, int dir_to_fd, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s, %d, %s)", __FUNCTION__,
         dir_from_fd, from.c_str(), dir_to_fd, to.c_str());

    if (::linkat(dir_from_fd, from.c_str(), dir_to_fd, to.c_str(), 0) != 0)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }
}


void FileSystem::SymLink(const FString& from, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %s)", __FUNCTION__, from.c_str(), to.c_str());

    if (::symlink(from.c_str(), to.c_str()) != 0)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }
}


void FileSystem::SymLinkAt(const FString& from, int dir_to_fd, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %d, %s)", __FUNCTION__,
         from.c_str(), dir_to_fd, to.c_str());

    if (::symlinkat(from.c_str(), dir_to_fd, to.c_str()) != 0)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }
}


FString FileSystem::ReadLink(const FString& path)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    FString ret;
    char buf[1024];  // MAXPATHLEN + 1
    int rc;

    if ((rc = ::readlink(path, buf, sizeof(buf))) == -1)
    {
        SystemCallUtil::ThrowErrNoException(errno);
    }

    buf[std::min<size_t>(rc, sizeof(buf) - 1)] = 0;
    ret = buf;
    return ret;
}


FString FileSystem::ResolveSymLink(const FString& path)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    struct stat st;
    char buf[1024];  // MAXPATHLEN + 1
    FString stmp, base(GetCWD()), ret(path);
    std::map<FString, bool> visited;
    unsigned n = 0;
    int rc;

    // resolve loop
    while (n++ < MAX_RESOLVE)
    {
        // check visited map
        if (visited.find(ret) != visited.end())
        {
            stmp.Format("FORTE_RESOLVE_SYMLINK_LOOP|||%s|||%s", 
                        path.c_str(), ret.c_str());
            throw EFileSystemResolveSymLink(stmp);
        }

        visited[ret] = true;

        // stat path
        if (LStat(ret, &st) != 0)
        {
            rc = errno;
            hlog(HLOG_DEBUG4, "FileSystem::%s(): unable to stat %s", __FUNCTION__, ret.c_str());
            stmp.Format("FORTE_RESOLVE_SYMLINK_BROKEN|||%s|||%s", path.c_str(), ret.c_str());
            throw EFileSystemResolveSymLink(stmp);
        }

        if (!S_ISLNK(st.st_mode)) return ret;

        // get base dir
        base = ResolveRelativePath(base, ret);
        base = base.Left(base.find_last_of('/'));

        // read link
        if ((rc = ::readlink(ret, buf, sizeof(buf))) == -1)
        {
            SystemCallUtil::ThrowErrNoException(errno);
        }

        buf[std::min<size_t>(rc, sizeof(buf) - 1)] = 0;
        ret = buf;

        // resolve link relative to dir containing the symlink
        ret = ResolveRelativePath(base, ret);
    }

    // too many iterations
    stmp.Format("FORTE_RESOLVE_SYMLINK_TOO_MANY|||%s|||%u", path.c_str(), 
                MAX_RESOLVE);
    throw EFileSystemResolveSymLink(stmp);
}


FString FileSystem::FullyResolveSymLink(const FString& path)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    std::vector<FString>::iterator pi;
    std::vector<FString> parts;
    std::map<FString, bool> visited, good;
    FString stmp, base, ret, partial, result;
    unsigned i = 0;
    bool done = false;
    struct stat st;
    int rc;

    // start with a full path
    ret = ResolveRelativePath(GetCWD(), path);

    // loop until path resolution results in no changes
    while (!done)
    {
        // check visited map
        if (visited.find(ret) != visited.end())
        {
            stmp.Format("FORTE_RESOLVE_SYMLINK_LOOP|||%s|||%s", 
                        path.c_str(), ret.c_str());
            throw EFileSystemResolveSymLink(stmp);
        }

        visited[ret] = true;

        // check for too many recursions
        if (i++ > MAX_RESOLVE)
        {
            stmp.Format("FORTE_RESOLVE_SYMLINK_TOO_MANY|||%s|||%u", 
                        path.c_str(), MAX_RESOLVE);
            throw EFileSystemResolveSymLink(stmp);
        }

        // init
        done = true;
        partial.clear();
        parts = ret.split("/");

        // loop over parts
        for (pi = parts.begin(); pi != parts.end(); ++pi)
        {
            // get partial path and base
            if (pi->empty()) continue;
            base = partial;
            if (base.empty()) base = "/";
            partial += "/" + (*pi);
            if (good.find(partial) != good.end()) continue;

            // hlog(HLOG_DEBUG4, "%s(%s) [step %u]: resolve %s",
            //__FUNCTION__, path.c_str(), i, partial.c_str());

            // stat partial path
            if (lstat(partial, &st) != 0)
            {
                rc = errno;
                stmp.Format("FORTE_RESOLVE_SYMLINK_BROKEN|||%s|||%s",
                            path.c_str(), partial.c_str());
                throw EFileSystemResolveSymLink(stmp);
            }

            // is partial path a link?
            if (S_ISLNK(st.st_mode))
            {
                result = ResolveRelativePath(base, ReadLink(partial));
            }
            else
            {
                result = partial;
                good[result] = true;
            }

            // did it change?
            if (partial != result)
            {
                done = false;
                ret = result;

                while ((++pi) != parts.end())
                {
                    ret += "/" + (*pi);
                }

                break;
            }
        }
    }

    // done
    hlog(HLOG_DEBUG4, "%s(%s) = %s", __FUNCTION__, path.c_str(), ret.c_str());
    return ret;
}


FString FileSystem::MakeRelativePath(const FString& base, const FString& path)
{
    std::vector<FString>::iterator bi, pi;
    std::vector<FString> b, p;
    FString ret;

    // get paths
    b = base.split("/");
    p = path.split("/");
    bi = b.begin();
    pi = p.begin();

    // find common paths
    while (bi != b.end() && pi != p.end() && (*bi == *pi))
    {
        ++bi;
        ++pi;
    }

    // generate path
    while (bi != b.end())
    {
        if (!ret.empty()) ret += "/..";
        else ret = "..";
        ++bi;
    }

    while (pi != p.end())
    {
        if (!ret.empty()) ret += "/";
        ret += *pi;
        ++pi;
    }

    // done
    hlog(HLOG_DEBUG4, "make_rel_path(%s, %s) = %s", base.c_str(), path.c_str(), ret.c_str());
    return ret;
}


FString FileSystem::ResolveRelativePath(const FString& base, 
                                        const FString& path)
{
    std::vector<FString>::reverse_iterator br;
    std::vector<FString>::iterator bi, pi;
    std::vector<FString> b, p;
    FString ret;

    // early exit?
    if (path.empty()) return base;
    if (path[0] == '/') return path;

    // get paths
    b = base.split("/");
    p = path.split("/");
    br = b.rbegin();
    pi = p.begin();

    // resolve path
    while (pi != p.end() && *pi == "..")
    {
        if (br != b.rend()) br++;  // backwards iteration
        ++pi;
    }

    // build path
    for (bi = b.begin(); br != b.rend(); ++bi, ++br)
    {
        if (bi->empty()) continue;
        ret += "/" + *bi;
    }

    while (pi != p.end())
    {
        ret += "/" + *pi;
        ++pi;
    }

    // done
    hlog(HLOG_DEBUG4, "ResolveRelativePath(%s, %s) = %s", base.c_str(), path.c_str(), ret.c_str());
    return ret;
}


void FileSystem::FileCopy(ProcRunner &pr, const FString& from, const FString& to, mode_t mode)
{
    hlog(HLOG_DEBUG4, "FileSystem::file_copy(%s, %s, %4o)",
         from.c_str(), to.c_str(), mode);
    FString command, to_dir, stmp;

    // make directory
    to_dir = to.Left(to.rfind('/'));
    MakeDir(to_dir, mode, true);

    // copy file
    command.Format("/bin/cp -f %s %s",
                   pr.ShellEscape(from).c_str(),
                   pr.ShellEscape(to).c_str());

    if (pr.Run(command, "", 0, PROC_RUNNER_NO_TIMEOUT) != 0)
    {
        stmp.Format("FORTE_COPY_FAIL|||%s|||%s", from.c_str(), to.c_str());
        throw EFileSystemCopy(stmp);
    }
}


FString FileSystem::FileGetContents(const FString& filename)
{
    hlog(HLOG_DEBUG4, "FileSystem::file_get_contents(%s)", filename.c_str());
    ifstream in(filename, ios::in | ios::binary);
    FString ret, stmp;
    char buf[16384];

    while (in.good())
    {
        in.read(buf, sizeof(buf));
        stmp.assign(buf, in.gcount());
        ret += stmp;
    }

    return ret;
}


void FileSystem::FilePutContents(const FString& filename, const FString& data, bool append)
{
    hlog(HLOG_DEBUG4, "FileSystem::file_put_contents(%s, [data])", 
         filename.c_str());
    ios_base::openmode mode=ios::out | ios::binary;

    if (append) {
        mode=mode | ios::app;
    }

    ofstream out(filename, mode);
    if (out.good()) out.write(data.c_str(), data.size());
}


void FileSystem::DeepCopy(const FString& source, const FString& dest, 
                          progress_callback_t progress_callback,
                          void *callback_data)
{
    InodeMap inode_map;
    uint64_t size_copied = 0;
    deepCopyHelper(source, dest, source, inode_map, size_copied, 
                   progress_callback, callback_data);
}


void FileSystem::deepCopyHelper(const FString& base_from, 
                                const FString& base_to, 
                                const FString& dir,
                                InodeMap &inode_map,
                                uint64_t &size_copied,
                                progress_callback_t progress_callback,
                                void *callback_data)
{
    hlog(HLOG_DEBUG4, "Filesystem::%s(%s, %s, %s)", __FUNCTION__,
         base_from.c_str(), base_to.c_str(), dir.c_str());
    struct dirent **entries;
    struct stat st;
    FString name, path, to_path, rel;
    int i, n;

    // first call?
    if (dir == base_from)
    {
        inode_map.clear();
        size_copied = 0;
    }

    // scan dir
    if ((n = scandir(dir, &entries, NULL, alphasort)) == -1) return;

    // copy this directory
    rel = dir.Mid(base_from.length());
    to_path = base_to + rel;

    if (stat(dir, &st) != 0)
    {
        hlog(HLOG_ERR, "Unable to perform deep copy on %s: directory is gone",
             path.c_str());
        throw EFileSystemCopy(FStringFC(), "FORTE_DEEP_COPY_FAIL|||%s", rel.c_str());
    }

    // create this directory
    try
    {
        MakeDir(path.Left(path.find_last_of('/')), 0777, true);  // make parent dirs first
        copyHelper(dir, to_path, st, inode_map, size_copied);
    }
    catch (Exception &e)
    {
        hlog(HLOG_ERR, "%s", e.GetDescription().c_str());
        throw EFileSystemCopy(FStringFC(), "FORTE_DEEP_COPY_FAIL|||%s", rel.c_str());
    }

    // copy the directory's entries
    for (i=0; i<n; free(entries[i++]))
    {
        name = entries[i]->d_name;
        if (name == "." || name == "..") continue;
        path = dir + "/" + name;
        rel = path.Mid(base_from.length());
        to_path = base_to + rel;

        if (lstat(path, &st) != 0)
        {
            hlog(HLOG_ERR, "Unable to perform deep copy on %s: directory is gone",
                 path.c_str());
            throw EFileSystemCopy(FStringFC(), "FORTE_DEEP_COPY_FAIL|||%s", rel.c_str());
        }
        else if (S_ISDIR(st.st_mode))
        {
            deepCopyHelper(base_from, base_to, path, inode_map, size_copied, 
                           progress_callback, callback_data);
        }
        else
        {
            try
            {
                copyHelper(path, to_path, st, inode_map, size_copied,
                           progress_callback, callback_data);
            }
            catch (Exception &e)
            {
                hlog(HLOG_ERR, "%s", e.GetDescription().c_str());
                throw EFileSystemCopy(FStringFC(), "FORTE_DEEP_COPY_FAIL|||%s", rel.c_str());
            }
        }
    }

    // done
    free(entries);
}


void FileSystem::Copy(const FString &from_path,
                      const FString &to_path,
                      progress_callback_t progress_callback,
                      void *callback_data)
{
    InodeMap inode_map;
    uint64_t size_copied = 0;
    struct stat st;

    if (stat(from_path, &st) != 0)
    {
        throw EFileSystemCopy(FStringFC(), "FORTE_COPY_FAIL|||%s|||%s",
                              from_path.c_str(), to_path.c_str());
    }

    copyHelper(from_path, to_path, st, inode_map, size_copied, progress_callback, callback_data);
}


void FileSystem::copyHelper(const FString& from_path, 
                            const FString& to_path,
                            const struct stat& st,
                            InodeMap &inode_map,
                            uint64_t &size_copied,
                            progress_callback_t progress_callback,
                            void *callback_data)
{
    struct timeval times[2];

    // what are we dealing with?
    if (S_ISDIR(st.st_mode))
    {
        // create directory
        MakeDir(to_path, st.st_mode & 0777);
        chown(to_path, st.st_uid, st.st_gid);
        times[0].tv_sec = st.st_atim.tv_sec;
        times[0].tv_usec = st.st_atim.tv_nsec / 1000;
        times[1].tv_sec = st.st_mtim.tv_sec;
        times[1].tv_usec = st.st_mtim.tv_nsec / 1000;
        utimes(to_path, times);
    }
    else if (S_ISLNK(st.st_mode))
    {
        // create symlink
        symlink(ReadLink(from_path), to_path);
    }
    else if (S_ISREG(st.st_mode))
    {
        bool copy = true;

        // has hard links?
        if (st.st_nlink > 1)
        {
            // check map
            InodeMap::iterator mi;

            if ((mi = inode_map.find(st.st_ino)) != inode_map.end())
            {
                // make hard link
                Link(mi->second, to_path);
                copy = false;
            }
            else
            {
                // save path so we can link it later
                inode_map[st.st_ino] = to_path;
            }
        }

        // do copy?
        if (copy)
        {
            const size_t buf_size = 65536; // 64 KB
            size_t i = 0, x;
            char buf[buf_size];

            ifstream in(from_path, ios::in | ios::binary);
            ofstream out(to_path, ios::out | ios::trunc | ios::binary);

            if (!in.good() || !out.good())
            {
                throw EFileSystemCopy("FORTE_COPY_FAIL|||" + from_path + "|||" + to_path);
            }

            while (in.good())
            {
                in.read(buf, buf_size);
                const size_t r = in.gcount();
                for (x=0; x<r && buf[x] == 0; x++);
                if (x == r) out.seekp(in.tellg(), ios::beg);            // leave a hole
                else out.write(buf, r);

                if ((progress_callback != NULL) && ((++i % 160) == 0))  // every 10 MB
                {
                    progress_callback(size_copied + in.tellg(), callback_data);
                }

                // TODO: replace this as g_shutdown has been removed!
                // if (g_shutdown)
                // {
                //     throw EFileSystemCopy("FORTE_COPY_FAIL_SHUTDOWN|||" + 
                //                           from_path + "|||" + to_path);
                // }
            }

            in.close();
            out.close();

            // set attributes
            truncate(to_path, st.st_size);
            chown(to_path, st.st_uid, st.st_gid);
            chmod(to_path, st.st_mode & 07777);
            times[0].tv_sec = st.st_atim.tv_sec;
            times[0].tv_usec = st.st_atim.tv_nsec / 1000;
            times[1].tv_sec = st.st_mtim.tv_sec;
            times[1].tv_usec = st.st_mtim.tv_nsec / 1000;
            utimes(to_path, times);
        }

        // count size copied
        size_copied += st.st_size;
    }
    else if (S_ISCHR(st.st_mode) ||
             S_ISBLK(st.st_mode) ||
             S_ISFIFO(st.st_mode) ||
             S_ISSOCK(st.st_mode))
    {
        // create special file
        if (mknod(to_path, st.st_mode, st.st_rdev) != 0)
        {
            hlog(HLOG_WARN, "copy: could not create special file: %s", to_path.c_str());
            // not worthy of an exception
        }
    }
    else
    {
        // skip unknown types
        hlog(HLOG_WARN, "copy: skipping file of unknown type %#x: %s",
             st.st_mode, to_path.c_str());
        // not worthy of an exception
    }

    // progress
    if (progress_callback != NULL) progress_callback(size_copied, callback_data);
}

