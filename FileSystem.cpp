// FileSystem.cpp

#include "Forte.h"
#include <fstream>
#include <vector>
#include <cstring>
#include <sys/time.h>

// constants
const unsigned MAX_RESOLVE = 1000;


// statics
CMutex FileSystem::s_mutex;
FileSystem* FileSystem::s_singleton = NULL;

FileSystem* FileSystem::get()
{
    // double-checked locking
    if (s_singleton == NULL)
    {
        CAutoUnlockMutex lock(s_mutex);
        if (s_singleton == NULL) s_singleton = new FileSystem();
    }

    return (FileSystem*)s_singleton;
}

FileSystem& FileSystem::getRef()
{
    FileSystem::get();

    if (s_singleton == NULL)
    {
        throw CForteEmptyReferenceException("FileSystem pointer is invalid");
    }

    return *s_singleton;
}


// ctor/dtor
FileSystem::FileSystem()
{
}


FileSystem::~FileSystem()
{
}


// helpers
FString FileSystem::strerror(int err) const
{
    char buf[256], *str;
    FString ret;
    str = strerror_r(err, buf, sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    ret = str;
    return ret;
}


// interface
FString FileSystem::getcwd()
{
    FString ret;
    char buf[1024];  // MAXPATHLEN + 1

    ::getcwd(buf, sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    ret = buf;
    return ret;
}


void FileSystem::touch(const FString& file)
{
    AutoFD fd;
    FString stmp;
    struct timeval tv[2];

    if ((fd = ::open(file, O_WRONLY | O_CREAT, 0666)) == AutoFD::NONE)
    {
        stmp.Format("FORTE_TOUCH_FAIL|||%s|||%s", file.c_str(), strerror(errno).c_str());
        throw CForteFileSystemTouchException(stmp);
    }

    if (gettimeofday(&(tv[0]), NULL) == -1)
    {
        stmp.Format("FORTE_TOUCH_FAIL|||%s|||%s", file.c_str(), strerror(errno).c_str());
        throw CForteFileSystemTouchException(stmp);
    }

    memcpy(&(tv[0]), &(tv[1]), sizeof(tv[0]));

    if (::futimes(fd, tv) == -1)
    {
        stmp.Format("FORTE_TOUCH_FAIL|||%s|||%s", file.c_str(), strerror(errno).c_str());
        throw CForteFileSystemTouchException(stmp);
    }
}


bool FileSystem::file_exists(const FString& filename)
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

bool FileSystem::is_dir(const FString& path)
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


int FileSystem::stat(const FString& path, struct stat *st)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    return ::stat(path.c_str(), st);
}


int FileSystem::lstat(const FString& path, struct stat *st)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    return ::lstat(path.c_str(), st);
}


int FileSystem::statat(int dir_fd, const FString& path, struct stat *st)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s)", __FUNCTION__, dir_fd, path.c_str());
    return ::fstatat(dir_fd, path.c_str(), st, 0);
}


int FileSystem::lstatat(int dir_fd, const FString& path, struct stat *st)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s)", __FUNCTION__, dir_fd, path.c_str());
    return ::fstatat(dir_fd, path.c_str(), st, AT_SYMLINK_NOFOLLOW);
}


int FileSystem::fstatat(int dir_fd, const FString& path, struct stat *st, int flags)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s)", __FUNCTION__, dir_fd, path.c_str());
    return ::fstatat(dir_fd, path.c_str(), st, flags);
}


void FileSystem::unlink(const FString& path, bool unlink_children,
                        progress_callback_t progress_callback,
                        void *callback_data)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %s)", __FUNCTION__,
         path.c_str(), (unlink_children ? "true" : "false"));
    FString stmp;

    // recursive delete?
    if (unlink_children)
    {
        struct dirent *entry;
        struct stat st;
        AutoFD dir;

        // inefficient but effective recursive delete algorithm
        // NOTE: use lstat() so we don't follow symlinks
        if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode))
        {
            if ((dir = opendir(path)).dir() != NULL)
            {
                while ((entry = readdir(dir)) != NULL)
                {
                    stmp = entry->d_name;

                    if (stmp != "." && stmp != "..")
                    {
                        unlink(path + "/" + entry->d_name, true, progress_callback, callback_data);
                    }
                }
            }
        }
    }

    // delete self
    unlink_helper(path);

    // callback?
    if (progress_callback != NULL) progress_callback(0, callback_data);
}


void FileSystem::unlinkat(int dir_fd, const FString& path)
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
        stmp.Format("FORTE_UNLINK_FAIL|||%s|||%s", path.c_str(), strerror(errno).c_str());
        throw CForteFileSystemUnlinkException(stmp);
    }
}


void FileSystem::unlink_helper(const FString& path)
{
    // hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    FString stmp;
    int err;

    if ((err = ::unlink(path)) != 0)
    {
        if (errno == ENOENT) return;
        if (errno == EISDIR) err = ::rmdir(path);
    }

    if (err != 0)
    {
        stmp.Format("FORTE_UNLINK_FAIL|||%s|||%s", path.c_str(), strerror(errno).c_str());
        throw CForteFileSystemUnlinkException(stmp);
    }
}


void FileSystem::rename(const FString& from, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %s)", __FUNCTION__, from.c_str(), to.c_str());
    FString stmp;

    if (::rename(from.c_str(), to.c_str()) != 0)
    {
        stmp.Format("FORTE_RENAME_FAIL|||%s|||%s|||%s",
                    from.c_str(), to.c_str(), strerror(errno).c_str());
        throw CForteFileSystemRenameException(stmp);
    }
}


void FileSystem::renameat(int dir_from_fd, const FString& from,
                          int dir_to_fd, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s, %d, %s)", __FUNCTION__,
         dir_from_fd, from.c_str(), dir_to_fd, to.c_str());
    FString stmp;

    if (::renameat(dir_from_fd, from.c_str(), dir_to_fd, to.c_str()) != 0)
    {
        stmp.Format("FORTE_RENAME_FAIL|||%s|||%s|||%s",
                    from.c_str(), to.c_str(), strerror(errno).c_str());
        throw CForteFileSystemRenameException(stmp);
    }
}


void FileSystem::mkdir(const FString& path, mode_t mode, bool make_parents)
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
        if (S_ISDIR(st.st_mode)) return;
        else throw CForteFileSystemMkdirException(FStringFC(), 
                                                  "FORTE_MKDIR_FAIL_IN_THE_WAY|||%s", 
                                                  path.c_str());
    }
    else
    {
        // create parent?
        if (make_parents)
        {
            parent = path.Left(path.find_last_of('/'));
            mkdir(parent, mode, true);
        }

        // make path
        ::mkdir(path, mode);
        err = errno;

        // validate path
        if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode))
        {
            if (err == 0)
            {
                throw CForteFileSystemMkdirException(FStringFC(), 
                                                     "FORTE_MKDIR_SUBSEQUENTLY_DELETED|||%s",
                                                     path.c_str());
            }

            stmp = strerror(err);
            throw CForteFileSystemMkdirException(FStringFC(), "FORTE_MKDIR_FAIL_ERR|||%s|||%s",
                             path.c_str(), stmp.c_str());
        }
    }
}


void FileSystem::mkdirat(int dir_fd, const FString& path, mode_t mode)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s, %04o)", __FUNCTION__, dir_fd, path.c_str(), mode);
    struct stat st;
    FString stmp;
    int err;

    // make path
    ::mkdirat(dir_fd, path, mode);
    err = errno;

    // validate path
    if (statat(dir_fd, path, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        if (err == 0)
        {
            throw CForteFileSystemMkdirException(FStringFC(), 
                                                 "FORTE_MKDIR_SUBSEQUENTLY_DELETED|||%s",
                                                 path.c_str());
        }

        stmp = strerror(err);
        throw CForteFileSystemMkdirException(FStringFC(), "FORTE_MKDIR_FAIL_ERR|||%s|||%s",
                                             path.c_str(), stmp.c_str());
    }
}


void FileSystem::link(const FString& from, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %s)", __FUNCTION__, from.c_str(), to.c_str());
    FString stmp;

    if (::link(from.c_str(), to.c_str()) != 0)
    {
        stmp.Format("FORTE_CREATE_HARD_LINK_FAIL|||%s|||%s|||%s",
                    from.c_str(), to.c_str(), strerror(errno).c_str());
        throw CForteFileSystemLinkException(stmp);
    }
}


void FileSystem::linkat(int dir_from_fd, const FString& from, int dir_to_fd, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%d, %s, %d, %s)", __FUNCTION__,
         dir_from_fd, from.c_str(), dir_to_fd, to.c_str());
    FString stmp;

    if (::linkat(dir_from_fd, from.c_str(), dir_to_fd, to.c_str(), 0) != 0)
    {
        stmp.Format("FORTE_CREATE_HARD_LINK_FAIL|||%s|||%s|||%s",
                    from.c_str(), to.c_str(), strerror(errno).c_str());
        throw CForteFileSystemLinkException(stmp);
    }
}


void FileSystem::symlink(const FString& from, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %s)", __FUNCTION__, from.c_str(), to.c_str());
    FString stmp;

    if (::symlink(from.c_str(), to.c_str()) != 0)
    {
        stmp.Format("FORTE_CREATE_SYMLINK_FAIL|||%s|||%s|||%s",
                    from.c_str(), to.c_str(), strerror(errno).c_str());
        throw CForteFileSystemSymLinkException(stmp);
    }
}


void FileSystem::symlinkat(const FString& from, int dir_to_fd, const FString& to)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s, %d, %s)", __FUNCTION__,
         from.c_str(), dir_to_fd, to.c_str());
    FString stmp;

    if (::symlinkat(from.c_str(), dir_to_fd, to.c_str()) != 0)
    {
        stmp.Format("FORTE_CREATE_SYMLINK_FAIL|||%s|||%s|||%s",
                    from.c_str(), to.c_str(), strerror(errno).c_str());
        throw CForteFileSystemSymLinkException(stmp);
    }
}


FString FileSystem::readlink(const FString& path)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    FString stmp, ret;
    char buf[1024];  // MAXPATHLEN + 1
    int rc;

    if ((rc = ::readlink(path, buf, sizeof(buf))) == -1)
    {
        stmp.Format("FORTE_READ_SYMLINK_FAIL|||%s|||%s", path.c_str(), strerror(errno).c_str());
        throw CForteFileSystemReadlinkException(stmp);
    }

    buf[std::min<size_t>(rc, sizeof(buf) - 1)] = 0;
    ret = buf;
    return ret;
}


FString FileSystem::resolve_symlink(const FString& path)
{
    hlog(HLOG_DEBUG4, "FileSystem::%s(%s)", __FUNCTION__, path.c_str());
    struct stat st;
    char buf[1024];  // MAXPATHLEN + 1
    FString stmp, base(getcwd()), ret(path);
    std::map<FString, bool> visited;
    unsigned n = 0;
    int rc;

    // resolve loop
    while (n++ < MAX_RESOLVE)
    {
        // check visited map
        if (visited.find(ret) != visited.end())
        {
            stmp.Format("FORTE_RESOLVE_SYMLINK_LOOP|||%s|||%s", path.c_str(), ret.c_str());
            throw CForteFileSystemResolveSymlinkException(stmp);
        }

        visited[ret] = true;

        // stat path
        if (lstat(ret, &st) != 0)
        {
            rc = errno;
            hlog(HLOG_DEBUG4, "FileSystem::%s(): unable to stat %s", __FUNCTION__, ret.c_str());
            stmp.Format("FORTE_RESOLVE_SYMLINK_BROKEN|||%s|||%s", path.c_str(), ret.c_str());
            throw CForteFileSystemResolveSymlinkException(stmp);
        }

        if (!S_ISLNK(st.st_mode)) return ret;

        // get base dir
        base = resolve_rel_path(base, ret);
        base = base.Left(base.find_last_of('/'));

        // read link
        if ((rc = ::readlink(ret, buf, sizeof(buf))) == -1)
        {
            stmp.Format("FORTE_RESOLVE_SYMLINK_FAIL|||%s|||%s|||%s", path.c_str(), ret.c_str(),
                        strerror(errno).c_str());
            throw CForteFileSystemResolveSymlinkException(stmp);
        }

        buf[std::min<size_t>(rc, sizeof(buf) - 1)] = 0;
        ret = buf;

        // resolve link relative to dir containing the symlink
        ret = resolve_rel_path(base, ret);
    }

    // too many iterations
    stmp.Format("FORTE_RESOLVE_SYMLINK_TOO_MANY|||%s|||%u", path.c_str(), MAX_RESOLVE);
    throw CForteFileSystemResolveSymlinkException(stmp);
}


FString FileSystem::fully_resolve_symlink(const FString& path)
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
    ret = resolve_rel_path(getcwd(), path);

    // loop until path resolution results in no changes
    while (!done)
    {
        // check visited map
        if (visited.find(ret) != visited.end())
        {
            stmp.Format("FORTE_RESOLVE_SYMLINK_LOOP|||%s|||%s", path.c_str(), ret.c_str());
            throw CForteFileSystemResolveSymlinkException(stmp);
        }

        visited[ret] = true;

        // check for too many recursions
        if (i++ > MAX_RESOLVE)
        {
            stmp.Format("FORTE_RESOLVE_SYMLINK_TOO_MANY|||%s|||%u", path.c_str(), MAX_RESOLVE);
            throw CForteFileSystemResolveSymlinkException(stmp);
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
                throw CForteFileSystemResolveSymlinkException(stmp);
            }

            // is partial path a link?
            if (S_ISLNK(st.st_mode))
            {
                result = resolve_rel_path(base, readlink(partial));
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


FString FileSystem::make_rel_path(const FString& base, const FString& path)
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


FString FileSystem::resolve_rel_path(const FString& base, const FString& path)
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
    hlog(HLOG_DEBUG4, "resolve_rel_path(%s, %s) = %s", base.c_str(), path.c_str(), ret.c_str());
    return ret;
}


void FileSystem::file_copy(const FString& from, const FString& to, mode_t mode)
{
    hlog(HLOG_DEBUG4, "FileSystem::file_copy(%s, %s, %4o)",
         from.c_str(), to.c_str(), mode);
    ProcRunner *proc = ProcRunner::get();
    FString command, to_dir, stmp;

    // make directory
    to_dir = to.Left(to.rfind('/'));
    mkdir(to_dir, mode, true);

    // copy file
    command.Format("/bin/cp -f %s %s",
                   proc->shell_escape(from).c_str(),
                   proc->shell_escape(to).c_str());

    if (proc->run(command, "", 0) != 0)
    {
        stmp.Format("FORTE_COPY_FAIL|||%s|||%s", from.c_str(), to.c_str());
        throw CForteFileSystemCopyException(stmp);
    }
}


FString FileSystem::file_get_contents(const FString& filename)
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


void FileSystem::file_put_contents(const FString& filename, const FString& data)
{
    hlog(HLOG_DEBUG4, "FileSystem::file_get_contents(%s, [data])", filename.c_str());
    ofstream out(filename, ios::out | ios::binary);
    if (out.good()) out.write(data.c_str(), data.size());
}


void FileSystem::deep_copy(const FString& source, const FString& dest, 
                           progress_callback_t progress_callback,
                           void *callback_data)
{
    InodeMap inode_map;
    uint64_t size_copied = 0;
    deep_copy_helper(source, dest, source, inode_map, size_copied, 
                     progress_callback, callback_data);
}


void FileSystem::deep_copy_helper(const FString& base_from, 
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
        throw CForteFileSystemCopyException(FStringFC(), "FORTE_DEEP_COPY_FAIL|||%s", rel.c_str());
    }

    // create this directory
    try
    {
        mkdir(path.Left(path.find_last_of('/')), 0777, true);  // make parent dirs first
        copy_helper(dir, to_path, st, inode_map, size_copied);
    }
    catch (CException &e)
    {
        hlog(HLOG_ERR, "%s", e.getDescription().c_str());
        throw CForteFileSystemCopyException(FStringFC(), "FORTE_DEEP_COPY_FAIL|||%s", rel.c_str());
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
            throw CForteFileSystemCopyException(FStringFC(), "FORTE_DEEP_COPY_FAIL|||%s", rel.c_str());
        }
        else if (S_ISDIR(st.st_mode))
        {
            deep_copy_helper(base_from, base_to, path, inode_map, size_copied, 
                             progress_callback, callback_data);
        }
        else
        {
            try
            {
                copy_helper(path, to_path, st, inode_map, size_copied,
                            progress_callback, callback_data);
            }
            catch (CException &e)
            {
                hlog(HLOG_ERR, "%s", e.getDescription().c_str());
                throw CForteFileSystemCopyException(FStringFC(), "FORTE_DEEP_COPY_FAIL|||%s", rel.c_str());
            }
        }
    }

    // done
    free(entries);
}


void FileSystem::copy(const FString &from_path,
                      const FString &to_path,
                      progress_callback_t progress_callback,
                      void *callback_data)
{
    InodeMap inode_map;
    uint64_t size_copied = 0;
    struct stat st;

    if (stat(from_path, &st) != 0)
    {
        throw CForteFileSystemCopyException(FStringFC(), "FORTE_COPY_FAIL|||%s|||%s",
                                            from_path.c_str(), to_path.c_str());
    }

    copy_helper(from_path, to_path, st, inode_map, size_copied, progress_callback, callback_data);
}


void FileSystem::copy_helper(const FString& from_path, 
                             const FString& to_path,
                             const struct stat& st,
                             InodeMap &inode_map,
                             uint64_t &size_copied,
                             progress_callback_t progress_callback,
                             void *callback_data)
{
    struct timeval times[2];
    FString stmp;

    // what are we dealing with?
    if (S_ISDIR(st.st_mode))
    {
        // create directory
        mkdir(to_path, st.st_mode & 0777);
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
        symlink(readlink(from_path), to_path);
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
                link(mi->second, to_path);
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
                throw CForteFileSystemCopyException("FORTE_COPY_FAIL|||" + from_path + "|||" + to_path);
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

                if (g_shutdown)
                {
                    throw CForteFileSystemCopyException("FORTE_COPY_FAIL_SHUTDOWN|||" + 
                                                        from_path + "|||" + to_path);
                }
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



//////////////// Advisory Locking ////////////////


FileSystem::AdvisoryLock::AdvisoryLock(int fd, off64_t start, off64_t len, short whence)
{
    m_fd = fd;
    m_lock.l_whence = whence;
    m_lock.l_start = start;
    m_lock.l_len = len;
}

/// getLock gets the first lock that blocks the lock description
///
FileSystem::AdvisoryLock FileSystem::AdvisoryLock::getLock(bool exclusive)
{
    AdvisoryLock lock(*this);
    if (exclusive)
        lock.m_lock.l_type = F_WRLCK;
    else
        lock.m_lock.l_type = F_RDLCK;
    fcntl(m_fd, F_GETLK, &lock);
    return lock;
}

/// sharedLock will return true on success, false if the lock failed
///
bool FileSystem::AdvisoryLock::sharedLock(bool wait)
{
    m_lock.l_type = F_RDLCK;
    if (fcntl(m_fd, wait ? F_SETLKW : F_SETLK, &m_lock) == -1)
        return false;
    return true;
}

/// exclusiveLock will return true on success, false if the lock failed
///
bool FileSystem::AdvisoryLock::exclusiveLock(bool wait)
{
    m_lock.l_type = F_WRLCK;
    if (fcntl(m_fd, wait ? F_SETLKW : F_SETLK, &m_lock) == -1)
        return false;
    return true;
}

/// unlock will remove the current lock
///
void FileSystem::AdvisoryLock::unlock(void)
{
    m_lock.l_type = F_UNLCK;
    fcntl(m_fd, F_SETLK, &m_lock);
}
